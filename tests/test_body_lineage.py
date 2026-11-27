import hashlib,importlib.util,json,unittest
from pathlib import Path
spec=importlib.util.spec_from_file_location('lineage',Path(__file__).resolve().parents[1]/'scripts/validate_body_lineage.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)

def fixture():
    parent='';lines=[]
    for ordinal in range(3):
        source={'schema':'halveth.virtual-body-lineage.v2','lineageId':'synthetic-test','entityId':'test-entity',
                'event':'birth' if ordinal==0 else 'explore','recordedAt':'2026-10-04T19:00:00Z',
                'simulationSeconds':ordinal*10,'parentDigest':parent,'generation':str(ordinal),'muscleReserve':1}
        if ordinal==0:source['originDigest']=hashlib.sha256(b'synthetic origin').hexdigest()
        payload=json.dumps(source,separators=(',',':'));digest=hashlib.sha256(payload.encode()).hexdigest()
        bases=''.join('ACGT'[(byte>>shift)&3] for byte in bytes.fromhex(digest) for shift in (6,4,2,0))
        record=source|{'sourcePayload':payload,'sourceDigest':digest,'newBases':bases,
                       'massKg':68,'paceCmS':74+ordinal*.01,'curiosity':.5,'reactionSeconds':.18}
        encoded=json.dumps(record,separators=(',',':')).encode();parent=hashlib.sha256(encoded).hexdigest();lines.append(encoded)
    return lines

class LineageTests(unittest.TestCase):
    def test_exact_chain_and_development(self):
        result=module.validate_bytes(b'\n'.join(fixture())+b'\n')
        self.assertEqual(result['records'],3);self.assertEqual(result['bases'],384);self.assertTrue(result['profileChanged'])
    def test_changed_parent_is_rejected(self):
        lines=fixture();record=json.loads(lines[1]);record['parentDigest']='0'*64;lines[1]=json.dumps(record).encode()
        with self.assertRaises(module.LineageError):module.validate_bytes(b'\n'.join(lines)+b'\n')
    def test_changed_source_bytes_are_rejected(self):
        lines=fixture();record=json.loads(lines[0]);record['sourcePayload']+=' ';lines[0]=json.dumps(record).encode()
        with self.assertRaises(module.LineageError):module.validate_bytes(b'\n'.join(lines)+b'\n')
    def test_altered_base_segment_is_rejected(self):
        lines=fixture();record=json.loads(lines[0]);record['newBases']='A'*128;lines[0]=json.dumps(record).encode()
        with self.assertRaises(module.LineageError):module.validate_bytes(b'\n'.join(lines)+b'\n')
    def test_partial_tail_is_rejected(self):
        with self.assertRaises(module.LineageError):module.validate_bytes(b'\n'.join(fixture()))

if __name__=='__main__':unittest.main()
