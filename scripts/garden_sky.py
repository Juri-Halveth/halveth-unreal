"""Bind one virtual site's 24-hour Sun trajectory to NASA/JPL Horizons.

Only the date and declared virtual coordinates are transmitted. No account,
user location, environment or local files are sent. Runtime uses this cache.
"""
from pathlib import Path
from datetime import datetime,timezone,timedelta
import argparse,csv,hashlib,json,os,tempfile,urllib.parse,urllib.request

ROOT=Path(__file__).resolve().parents[1]

def _stage_file(destination, data):
    """Prepare a closed, flushed sibling for atomic replacement."""
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(
            dir=destination.parent, prefix='.'+destination.name+'.', suffix='.tmp', delete=False
        ) as stream:
            temporary = Path(stream.name)
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        return temporary
    except BaseException:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
        raise


def _replace_files(replacements):
    """Replace each file atomically, rolling back earlier replacements on error.

    This is recoverable on an I/O failure, not a cross-directory crash transaction.
    """
    staged, backups, published = {}, {}, []
    rollback_failed = False
    try:
        for destination, data in replacements.items():
            destination.parent.mkdir(parents=True, exist_ok=True)
            backups[destination] = (
                _stage_file(destination, destination.read_bytes()) if destination.exists() else None
            )
            staged[destination] = _stage_file(destination, data)
        try:
            for destination, temporary in staged.items():
                os.replace(temporary, destination)
                published.append(destination)
        except BaseException as error:
            for destination in reversed(published):
                try:
                    backup = backups[destination]
                    if backup is None:
                        destination.unlink()
                    else:
                        os.replace(backup, destination)
                except OSError:
                    rollback_failed = True
            if rollback_failed:
                raise RuntimeError('Sky cache rollback failed; recovery backups were retained.') from error
            raise
    finally:
        for temporary in staged.values():
            temporary.unlink(missing_ok=True)
        if not rollback_failed:
            for backup in backups.values():
                if backup is not None:
                    backup.unlink(missing_ok=True)


def sync(day=None):
    day=day or datetime.now(timezone.utc).date().isoformat()
    start=datetime.fromisoformat(day).replace(tzinfo=timezone.utc)
    stop=start+timedelta(days=1)
    params={'format':'json','COMMAND':"'10'",'OBJ_DATA':"'YES'",'MAKE_EPHEM':"'YES'",'EPHEM_TYPE':"'OBSERVER'",'CENTER':"'coord@399'",'COORD_TYPE':"'GEODETIC'",'SITE_COORD':"'0,45,0'",'START_TIME':"'"+start.strftime('%Y-%m-%d')+"'",'STOP_TIME':"'"+stop.strftime('%Y-%m-%d')+"'",'STEP_SIZE':"'1 h'",'QUANTITIES':"'4'",'CSV_FORMAT':"'YES'",'ANG_FORMAT':"'DEG'",'APPARENT':"'AIRLESS'"}
    url='https://ssd.jpl.nasa.gov/api/horizons.api?'+urllib.parse.urlencode(params)
    request=urllib.request.Request(url,headers={'User-Agent':'HALVETH-Portal-Garden-Sky/1.0'})
    with urllib.request.urlopen(request,timeout=30) as response:
        data=response.read(512000)
        if response.status!=200 or len(data)>=512000:raise ValueError('Unbounded or unsuccessful Horizons response')
    doc=json.loads(data)
    if not isinstance(doc,dict):raise ValueError('Unexpected Horizons response object')
    signature=doc.get('signature')
    if not isinstance(signature,dict) or signature.get('version') not in {'1.2','1.3'}:raise ValueError('Horizons schema changed; review the primary API documentation')
    print('HORIZONS_API_SIGNATURE',doc.get('signature'),flush=True)
    text=doc.get('result','')
    if not isinstance(text,str) or '$$SOE' not in text or '$$EOE' not in text:raise ValueError('Horizons ephemeris table missing')
    lines=text.split('$$SOE',1)[1].split('$$EOE',1)[0].strip().splitlines()
    samples=[]
    for row in csv.reader(lines):
        if len(row)<5:raise ValueError('Unexpected ephemeris row')
        date=datetime.strptime(row[0].strip(),'%Y-%b-%d %H:%M').replace(tzinfo=timezone.utc)
        azimuth=float(row[3]);altitude=float(row[4])
        if not 0<=azimuth<=360 or not -90<=altitude<=90:raise ValueError('Invalid Sun angles')
        samples.append({'utc':date.isoformat().replace('+00:00','Z'),'unixSeconds':int(date.timestamp()),'azimuthDegrees':azimuth,'altitudeDegrees':altitude})
    if len(samples)!=25 or any(samples[i+1]['unixSeconds']-samples[i]['unixSeconds']!=3600 for i in range(24)):raise ValueError('Expected complete 25-point, hourly trajectory')
    if samples[0]['unixSeconds']!=int(start.timestamp()) or samples[-1]['unixSeconds']!=int(stop.timestamp()):raise ValueError('Horizons trajectory does not cover the requested date')
    output={'schema':'halveth.nasa-sun.v1','source':'NASA/JPL Horizons','apiSignature':doc['signature'],'requestUrl':url,'fetchedAtUtc':datetime.now(timezone.utc).isoformat(),'rawResponseSha256':hashlib.sha256(data).hexdigest(),'target':'Sun (10)','observer':{'kind':'DECLARED_VIRTUAL_SITE','longitudeDegrees':0,'latitudeDegrees':45,'altitudeKilometres':0},'coverageHours':24,'sampleIntervalSeconds':3600,'rendering':'Direction interpolation between hourly samples; not a live weather service','samples':samples}
    encoded=(json.dumps(output,indent=2,allow_nan=False)+'\n').encode('utf8')
    folder=ROOT/'assets/sky'
    _replace_files({
        folder/'horizons-response.json': data,
        folder/'ephemeris.json': encoded,
        ROOT/'Content/sky/ephemeris.json': encoded,
    })
    print('NASA_SKY_BOUND',len(samples),'samples',day,'virtual site 45N, 0E',flush=True)
    return output
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('mode',choices=['sync']);parser.add_argument('--date');args=parser.parse_args();sync(args.date)
