#!/usr/bin/python
import sys, os, itertools
sys.path.append (os.path.expanduser ("~/engine/src/drinkless/python"))
from drinkless import *

import stat

tests_common_dir = os.path.join (DL_PATH_SRC, "PHP/tests/phpt")
class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'

    def disable(self):
        self.HEADER = ''
        self.OKBLUE = ''
        self.OKGREEN = ''
        self.WARNING = ''
        self.FAIL = ''
        self.ENDC = ''
def prepare_test (src, dest, init_code = ""):
  src_f = open (src, "r")
  dest_f = open (dest, "w")

  dest_f.write (init_code)

  cur = src_f.readline()
  if cur and cur[0] != '@':
    dest_f.write (cur)
  while True:
    cur = src_f.readline()
    if not cur:
      break
    dest_f.write (cur)

  os.fchmod (dest_f.fileno(), stat.S_IRUSR)
  src_f.close()
  dest_f.close()

def in_tmp (path):
  return os.path.join ("tmp/", os.path.basename (path))

supported_modes = ["php", "kphp", "hhvm", "custom", "none"];
def run_test_mode_cmd (mode, cmd):
  ans_path = "tmp/ans"
  perf_path = "tmp/perf"
  err_path = "tmp/err";
  full_cmd = " ".join ([
    #"/usr/bin/time -o", perf_path, "-f \"%e %M\" ",
    os.path.join (DL_PATH_SRC, "objs/bin/dltime"), "-o", perf_path,
      cmd, 
      ">", ans_path,
      "2>", err_path
      ])
  check_call (full_cmd, shell = True)
  return (ans_path, perf_path)

def run_test_mode_php (path):
  cmd = "php -n -d memory_limit=3072M %s" % (path)
  return run_test_mode_cmd ("php", cmd)
def run_test_mode_hhvm (path):
  cmd = "hhvm  -v\"Eval.Jit=true\" %s" % (path)
  return run_test_mode_cmd ("hhvm", cmd)
def run_test_mode_kphp (path):
  make_cmd = '../kphp.sh -S -I tmp/ -M cli -o tmp/main %s 2> tmp/kphp_err > tmp/kphp_out' % (path)
  check_call (make_cmd, shell = True)
  cmd = "tmp/main"
  return run_test_mode_cmd ("kphp", cmd)
def run_test_mode_custom (path):
  cmd = "..."
  return run_test_mode_cmd ("custom", cmd)

def get_ans_path (path):
  path = os.path.relpath (path, tests_common_dir)
  path = string.replace (path, "/", "@")
  return os.path.join ("answers/", path + ".ans")
def get_perf_path (path, mode):
  path = os.path.relpath (path, tests_common_dir)
  path = string.replace (path, "/", "@")
  return os.path.join ("perf", mode, path + ".ans")

def perf_cmp_time (baseline, current):
  baseline = float (baseline)
  current = float (current)
  eps = 0.01
  if baseline < eps:
    baseline = eps
  if current < eps:
    current = eps
  ratio = baseline / current
  diff = abs (baseline - current)
  res = "%.2lf" % ratio
  if (ratio > 0.9 and ratio < 1.1 and diff < 0.1) or (current < 0.0101 and baseline < 0.0101) or (diff < 0.004):
    res = bcolors.OKBLUE + res + bcolors.ENDC
  elif current < baseline:
    res = bcolors.OKGREEN + res + bcolors.ENDC
  else:
    res = bcolors.FAIL + res + bcolors.ENDC
  return res
 
def perf_cmp_memory (baseline, current):
  baseline = float (baseline)
  current = float (current)
  eps = 1000
  if baseline < eps:
    baseline = eps
  if current < eps:
    current = eps
  ratio = baseline / current
  diff = abs (baseline - current)
  res = "%.2lf" % ratio
  if (ratio > 0.98 and ratio < 1.02):
    res = bcolors.OKBLUE + res + bcolors.ENDC
  elif current < baseline:
    res = bcolors.OKGREEN + res + bcolors.ENDC
  else:
    res = bcolors.FAIL + res + bcolors.ENDC
  return res

total_cnt = 0
wa_cnt = 0
wa_tests = []
def run_test_mode (test, mode, perf_read_only = True, 
    ans_read_only = True, compare_with_modes = [], keep_going = False):
  global total_cnt
  global ok_cnt
  global wa_cnt
  no_php = "no_php" in test.tags
  if no_php and mode == "php":
    return 1 #skip this test

  perfs = []
  was_run = False
  if mode != "none":
    was_run = True
    total_cnt += 1
    perf_path = get_perf_path (test.path, mode)
    test_path = in_tmp (test.path)
    for x in itertools.chain ([test.path], test.to_copy):
      prepare_test (x, in_tmp (x))
    
    try:
      if mode == "php":
        res = run_test_mode_php (test_path)
      elif mode == "kphp":
        res = run_test_mode_kphp (test_path)
      elif mode == "hhvm":
        res = run_test_mode_hhvm (test_path)
      elif mode == "custom":
        res = run_rest_mode_hhvm (test_path)
    except:
      print "%sFailed to run [%s] [%s]%s" % (bcolors.FAIL, test.short_path,
          mode, bcolors.ENDC)
      return
      sys.exit (1)

    cur_ans_path, cur_perf_path = res

    ok = True
    if ans_read_only:
      if not test.check_answer (cur_ans_path, mode, keep_going):
        wa_cnt += 1
        wa_tests.append (test)
        ok = False
    else:
      test.save_answer (cur_ans_path, mode)

    if ok and not perf_read_only:
      test.save_perf (cur_perf_path, mode)
    perfs.append (("now." + mode, open (cur_perf_path, "r").read().split()))

  for other_mode in compare_with_modes:
    perf = test.get_perf (other_mode)
    if perf:
      perfs.insert (0, (other_mode, perf))

  if perfs:
    if not was_run:
      print "Test [%s]" % test.short_path
    base_mode, base_perf = perfs[0]
    for mode, perf in perfs:
      print "[%10s]\ttime %s(%s)\tmemory %s(%s)" % (
          mode, 
          perf[0], perf_cmp_time (base_perf[0], perf[0]), 
          perf[1], perf_cmp_memory (base_perf[1], perf[1]))
    print "----------------------------------------"

all_tags = set()
class Test:
  def __init__ (self, path, tags = set()):
    global all_tags

    self.path = path
    self.short_path = os.path.relpath (self.path, tests_common_dir)
    f = open (path, "r")
    header = f.readline()
    f.close()
    if header[0] == '@':
      self.tags = set (header[1:].split())
    else:
      self.tags = set (["none"])
    self.tags |= tags
    all_tags |= self.tags

    prefix = string.rsplit (path, '.', 1)[0]
    self.to_copy = glob (prefix + "*.php")
    self.to_copy.remove (path)

    self.ans_path = get_ans_path (self.path)
    if not os.path.isfile (self.ans_path):
      self.tags |= set (["no_ans"])
      all_tags |= set (["no_ans"])
  def check_answer (self, ans_path, mode, keep_going = False):
    if not os.path.isfile (self.ans_path):
      print "No answer for test [%s] found: %sNO ANSWER%s" % (self.short_path,
      bcolors.FAIL, bcolors.ENDC)
      if not keep_going:
        sys.exit (1)
      return False
    cmd = " ".join (["diff", self.ans_path, ans_path])
    err = call (cmd, shell = True)
    if err:
      print "Test [%s] [%s]: %sWA%s" % (self.short_path, mode, bcolors.FAIL,
          bcolors.ENDC)
      if not keep_going:
        sys.exit (1)
      return False
    else:
      print "Test [%s] [%s]: %sOK%s" % (self.short_path, mode, bcolors.OKGREEN,
          bcolors.ENDC)
      open(test.path + ".ok", 'w').close()
      return True

  def save_answer (self, ans_path, mode):
    if os.path.exists (self.ans_path):
      print "Answer for [%s] already exists" % self.short_path
    else:
      cmd = " ".join (["cp", ans_path, self.ans_path])
      check_call (cmd, shell = True)
      print "Answer for [%s] is generated by [%s]" % (self.short_path, mode)

  def save_perf (self, perf_path, mode):
    target_perf_path = get_perf_path (self.path, mode)
    perf = self.get_perf (mode)
    if os.path.exists (perf_path):
      f = open (perf_path, "r")
      new_perf = f.read().split()
      f.close()
    else:
      new_perf = None
      print "Perf file [%s] not exists" % perf_path

    if perf:
      if new_perf:
        perf[0] = min (perf[0], new_perf[0])
        perf[1] = min (perf[1], new_perf[1])

        f = open (target_perf_path, "w")
        f.write(" ".join (perf))
        f.close()
    else:
      cmd = " ".join (["cp", perf_path, target_perf_path])
      check_call (cmd, shell = True)

  def get_perf (self, mode):
    perf_path = get_perf_path (self.path, mode)
    if os.path.exists (perf_path):
      f = open (perf_path, "r")
      perf = f.read().split()
      f.close()
      return perf
    return None

def get_tests (tests, tags = []):
  tests = sorted (tests)
  tests = [Test (x, set(tags)) for x in tests]
  return tests

def get_tests_by_mask (mask, tags = []):
  return get_tests (glob (mask), tags);

def remove_ok_tests (tests, force = False, save_files = False):
  tmp_tests = []
  for x in tests:
    was = x.path + ".ok";
    if os.path.exists (was):
      if force:
        if not save_files:
          os.remove (was)
        tmp_tests.append (x)

    else:
      tmp_tests.append (x)
  return tmp_tests

def usage():
  print "DL_PHP tester"
  print "usage: python tester.py [-t <tag>] [-l]"
  print "\t-A --- generate answer if none"
  print "\t-P ---  rewrite perf files"
  print "\t-a<tag> --- test must have given <tag>"
  print "\t-d<tag> --- test mustn't have given <tag>"
  print "\t-c<mode> --- compare with modes"
  print "\t-k --- keep going"
  print "\t-l --- just print test names"
  print "\t-m<mode> --- run in mode (kphp, php, hhvm, custom, none)"
  print "\t-i --- test till infinity"

print_tests = False

try:
  opts, args = getopt(sys.argv[1:], "Aa:c:d:klm:PRhfi")
except:
  usage()
  exit(0)

need_tags = set()
bad_tags = set()

init_code = ""
run_mode = "kphp"
force = False
inf_flag = False
ans_read_only = True
perf_read_only = True
compare_with_modes = []
keep_going = False
for o, a in opts:
  if o == "-A":
    ans_read_only = False
  elif o == "-P":
    perf_read_only = False
  elif o == "-a":
    need_tags.add (a)
  elif o == '-c':
#    assert a in supported_modes
    compare_with_modes.append (a)
  elif o == "-d":
    bad_tags.add (a)
  elif o == "-l":
    print_tests = True
  elif o == "-k":
    keep_going = True
  elif o == "-m":
    assert a in supported_modes
    run_mode = a
  elif o == "-R":
    ans_rewrite = True
  elif o == "-h":
    usage()
    exit(0)
  elif o == "-f":
    force = True;
  elif o == "-i":
    inf_flag = True;
  else:
    usage()
    exit(0)

manual_tests = []
if args:
  manual_tests = get_tests (args, ["manual"])
  need_tags.add ("manual")

dl_tests = get_tests_by_mask (DL_PATH_SRC + "PHP/tests/phpt/dl/*.php", ["dl"])
dl_switch_tests = get_tests_by_mask (DL_PATH_SRC + "PHP/tests/phpt/dl/switch/*.php", ["dl", "switch"])
#hiphop_basic_tests = get_tests_by_mask (DL_PATH_SRC + "PHP/tests/phpt/hiphop/tests/basic/*.phpt", ["hiphop", "hiphop_basic"]);
phc_parsing_tests = get_tests_by_mask (DL_PATH_SRC + "PHP/tests/phpt/phc/parsing/*.php", ["phc", "parsing"]);
phc_codegen_tests = get_tests_by_mask (DL_PATH_SRC + "PHP/tests/phpt/phc/codegen/*.php", ["phc", "codegen"]);
#benchmark_tests = get_tests_by_mask (DL_PATH_SRC + "PHP/tests/phpt/benchmarks/*/*.php", ["benchmark", "bench"]);

tests = []
tests.extend (dl_tests)
tests.extend (dl_switch_tests)
#tests.extend (hiphop_basic_tests)
tests.extend (manual_tests)
tests.extend (phc_parsing_tests)
tests.extend (phc_codegen_tests)
#tests.extend (benchmark_tests)

print all_tags

tests = filter (lambda x: need_tags <= x.tags and (not (bad_tags & x.tags)), tests)
tests = remove_ok_tests (tests, force, print_tests)
used_tags = reduce (lambda x, y: x | y.tags, tests, set())

print used_tags

if print_tests:
  for x in tests:
    print x.path
  print len (tests)
  exit(0)

run_make()

check_call ("rm -rf tmp", shell = True)
check_call ("mkdir tmp", shell = True)
if not os.path.exists("answers"):
  check_call ("mkdir answers", shell = True)
if not os.path.exists("perf"):
  check_call ("mkdir perf", shell = True)
for mode in supported_modes:
  path = os.path.join ("perf", mode)
  if not os.path.exists (path):
    check_call (" ".join (["mkdir", path]), shell = True);

if run_mode == "php":
  tests = filter (lambda x: "no_php" not in x.tags, tests)

for test in tests:
  check_call ("rm -rf tmp/*", shell = True)
  ok = run_test_mode (test, run_mode, ans_read_only = ans_read_only, 
      perf_read_only = perf_read_only, compare_with_modes = compare_with_modes,
      keep_going = keep_going)


print "Total tests runned: %d" % total_cnt
if wa_cnt:
  print "%sFAILED: %d%s" % (bcolors.FAIL, wa_cnt, bcolors.ENDC)
  for test in wa_tests:
    print "\t%s[%s]%s" % (bcolors.FAIL, test.short_path, bcolors.ENDC)
