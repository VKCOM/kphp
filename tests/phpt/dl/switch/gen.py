import sys, os
sys.path.append (os.path.expanduser ("~/engine/src/drinkless/python"))
from drinkless import *
from random import *

str_n = 50
break_n = 100
print_n = 30
q_n = 100

def rand_str():
  n = randint (1, 10)
  s = ""
  for t in xrange (n):
    s += chr (randint (ord ('a'), ord ('z')))
  return s

qs = list (set (rand_str() for x in xrange (q_n)))

strs = sample (qs, str_n)

code = []
code.extend ('case "%s":' % x for x in strs)
code.extend ('print "%d\\n";' % x for x in xrange (print_n))
#code.extend ('break;' for x in xrange (break_n))

shuffle (code)

print "@benchmark"
print "<?"

print "function f($s) {"
print "  switch ($s) {"
print "    case '_dummy_':"
for x in code:
  print "    %s" % x
  print "break;"
print "  }"
print "}"


print "$arr = array (%s);" % ",".join (map (lambda x: '"%s"' % x, qs))

print "for ($i = 0; $i < 1000; $i++)"
print "foreach ($arr as $x) {"
print '  print "---------------------------------------\\n";'
print '  print "test {$x}\\n";'
print "  f ($x);"
print "}"


print "?>"

