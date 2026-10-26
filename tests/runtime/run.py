import pathlib,struct,zlib,tempfile,subprocess
root=pathlib.Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory() as directory:
 p=pathlib.Path(directory)
 def record(generation,text):
  b=bytearray(168); b[:4]=b'RSM1';struct.pack_into('<II',b,4,generation,123)
  b[12:28]=bytes.fromhex('7ef1766a5a364d00a000000000000001')
  b[28:32]=bytes([0,6,len(text),0]);struct.pack_into('<HH',b,32,40,40)
  b[36:36+len(text)]=text.encode();b[100:106]=b'test#1';struct.pack_into('<I',b,164,zlib.crc32(b[:164]));return b
 (p/'first').write_bytes(record(1,'test#1'));(p/'next').write_bytes(record(2,'changed@example.com'))
 module=root/'modules/roba-runtime'
 subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wno-unused-parameter','-Werror','-I',str(module),str(module/'core.c'),str(root/'tests/runtime/core-test.c'),'-o',str(p/'test')],check=True)
 result=subprocess.run([str(p/'test'),str(p/'first'),str(p/'next')],check=True,capture_output=True)
 assert len(result.stdout.splitlines())==190
 print('CRC, validation, stale revisions, full persistence/reload and all 169 interrupted-write boundaries passed.')
