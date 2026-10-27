import unittest
from boot_test import decode_vga,qcode
class Helpers(unittest.TestCase):
 def test_decode(self):
  b=bytearray(b' \x0f'*2000)
  for i,c in enumerate(b'Kernel ready.'):b[i*2]=c
  self.assertEqual(decode_vga(b).splitlines()[0],'Kernel ready.')
  self.assertEqual(len(decode_vga(b).split('\n')),25)
 def test_bad_length(self):
  with self.assertRaises(ValueError):decode_vga(bytes(20))
 def test_keys(self):
  for c in 'mkdir /test\ncat /probe.txt\n':self.assertIsNotNone(qcode(c))
  self.assertEqual(qcode('/'),'slash')
  self.assertIsNone(qcode('A'))
if __name__=='__main__':unittest.main()
