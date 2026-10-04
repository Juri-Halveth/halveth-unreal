# SPDX-License-Identifier: MIT
"""Validate exact UTF-8 game-lineage records, source digests and base segments."""
from pathlib import Path
import argparse,hashlib,json,math,re

class LineageError(ValueError):pass

def strict_object(pairs):
    out={}
    for key,value in pairs:
        if key in out:raise LineageError('Duplicate JSON field')
        out[key]=value
    return out

def load(text):
    return json.loads(text,object_pairs_hook=strict_object,
                      parse_constant=lambda value:(_ for _ in ()).throw(LineageError('Nonfinite JSON number')))

def validate_bytes(data):
    if not data or not data.endswith(b'\n'):raise LineageError('Complete LF-terminated records required')
    parent='';lineage=None;entity=None;events=[];profiles=[];base_count=0
    for ordinal,line in enumerate(data.split(b'\n')[:-1]):
        record=load(line.decode('utf-8'));payload=load(record['sourcePayload'])
        if record['schema']!='halveth.virtual-body-lineage.v2':raise LineageError('Schema requires v2')
        source_fields={'schema','lineageId','entityId','event','recordedAt','simulationSeconds','parentDigest','generation','muscleReserve'}
        if ordinal==0:source_fields.add('originDigest')
        output_fields={'sourcePayload','sourceDigest','newBases','massKg','paceCmS','curiosity','reactionSeconds'}
        if set(payload)!=source_fields or set(record)!=source_fields|output_fields:raise LineageError('Record fields differ from bound contract')
        if any(payload[key]!=record[key] for key in source_fields):raise LineageError('Source fields changed')
        if record['generation']!=str(ordinal) or record['parentDigest']!=parent:raise LineageError('Parent or generation discontinuity')
        if ordinal==0:
            lineage=record['lineageId'];entity=record['entityId']
            if record['event']!='birth' or not re.fullmatch('[0-9a-fA-F]{64}',record['originDigest']):raise LineageError('Invalid birth binding')
        if record['lineageId']!=lineage or record['entityId']!=entity:raise LineageError('Entity or lineage changed')
        digest=hashlib.sha256(record['sourcePayload'].encode('utf-8')).hexdigest()
        if digest!=record['sourceDigest'].lower():raise LineageError('Source digest differs')
        bases=''.join('ACGT'[(byte>>shift)&3] for byte in bytes.fromhex(digest) for shift in (6,4,2,0))
        if record['newBases']!=bases:raise LineageError('Genome segment differs from digest')
        for key in ('simulationSeconds','muscleReserve','massKg','paceCmS','curiosity','reactionSeconds'):
            if type(record[key]) not in (int,float) or not math.isfinite(record[key]):raise LineageError('Invalid numeric state')
        if record['massKg']<=0 or record['paceCmS']<=0 or record['reactionSeconds']<=0:raise LineageError('Nonpositive physical profile')
        parent=hashlib.sha256(line).hexdigest();base_count+=len(bases);events.append(record['event'])
        profiles.append({key:record[key] for key in ('massKg','paceCmS','curiosity','reactionSeconds')})
    return {'schema':'halveth.lineage-validation.v1','lineageId':lineage,'entityId':entity,
            'records':len(events),'bases':base_count,'events':events,'headDigest':parent,
            'profileChanged':profiles[0]!=profiles[-1],'initialProfile':profiles[0],'currentProfile':profiles[-1],
            'parentChainPassed':True,'sourceDigestsPassed':True,'genomeMappingPassed':True}

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('path',type=Path);args=parser.parse_args()
    print(json.dumps(validate_bytes(args.path.read_bytes()),indent=2))

if __name__=='__main__':main()
