import unittest,tempfile,struct
from pathlib import Path
from disk_tools import Store,N,RECORD,SIZE,MAGIC,checksum
class DiskTests(unittest.TestCase):
 def setUp(self):
  self.temp=tempfile.TemporaryDirectory();self.path=Path(self.temp.name)/'disk.img'
  raw=RECORD.pack(1,1,0,b'',b'')+bytes(RECORD.size*(N-1))
  image=bytearray(1024*1024);struct.pack_into('<III',image,0,MAGIC,SIZE,checksum(raw));image[512:512+SIZE]=raw;self.path.write_bytes(image)
 def tearDown(self):self.temp.cleanup()
 def test_roundtrip_move_and_backup(self):
  s=Store(self.path);s.create('/Documents',True);s.create('/notes.txt',False,b'Hello')
  s.move('/notes.txt','/Documents/');backup=s.commit();self.assertTrue(backup.exists())
  s=Store(self.path);self.assertEqual(s.nodes[s.resolve('/Documents/notes.txt')]['data'],b'Hello')
  s.move('/Documents/notes.txt','/');s.commit();s=Store(self.path)
  self.assertEqual(s.nodes[s.resolve('/notes.txt')]['data'],b'Hello')
 def test_collisions_and_cycles(self):
  s=Store(self.path);s.create('/a',True);s.create('/a/b',True)
  with self.assertRaises(ValueError):s.move('/a','/a/b')
  with self.assertRaises(ValueError):s.create('/a',False)
 def test_invalid_text(self):
  s=Store(self.path)
  for data in [b'x'*1024,b'a\0b',b'\xff']:
   with self.assertRaises((ValueError,UnicodeError)):s.create('/bad',False,data)
 def test_corruption(self):
  b=bytearray(self.path.read_bytes());b[520]^=1;self.path.write_bytes(b)
  with self.assertRaises(ValueError):Store(self.path)
 def test_concurrent_change(self):
  s=Store(self.path);b=bytearray(self.path.read_bytes());b[-1]=1;self.path.write_bytes(b)
  with self.assertRaises(ValueError):s.commit()
if __name__=='__main__':unittest.main()
