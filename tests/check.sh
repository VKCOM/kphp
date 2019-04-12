#!/bin/bash
pushd `pwd`

BRANCH_NAME=`git rev-parse --abbrev-ref HEAD`
echo -e "\n\nRESET YOUR STATE OF ~/engine to your origin/ branch\n\n" && sleep 1
cd ~/engine
git rebase --abort
git fetch --all && git reset --hard origin/$BRANCH_NAME

echo -e "\n\nCHECK THAT ALL COMMITS ARE COMPILED\n\n" && sleep 1
if [ "$#" -ne 1 ]; then
  GIT_SEQUENCE_EDITOR=: git rebase -q `git log -1 origin/master --pretty=format:"%h" --no-patch` -i --exec 'make g=1 -j php -C ~/engine' || exit 1
fi

echo -e "\n\nGENERATE TESTS\n\n" && sleep 1
cd PHP/tests
./tester.py -a ok -m php -A || exit 1

echo -e "\n\nRUN OK TESTS\n\n" && sleep 1
./tester.py -a ok -f || exit 1

echo -e "\n\nRUN kphp_should_fail TESTS\n\n" && sleep 1
./tester.py -a kphp_should_fail -f || exit 1


echo -e "\n\nPULL www\n\n" && sleep 1
cd ~/www
git pull

echo -e "\n\nGENERATE www BY MASTER\n\n" && sleep 1
cd ~/engine && git checkout master && git pull && make -j php -C ~/engine
KPHP_USE_MAKE=0 KPHP_BUILD_ARGS='build_args:-vvv' KPHP_CUSTOM_LIBS="kphp_custom_libs:/home/`whoami`/my-kphp-libs/all_ml/" vk mykphp

echo -e "\n\nCOPY GENERATED www\n\n" && sleep 1
rm -rf ~/kphp_new
cp -r /var/kphp/`whoami`/bin-my/kphp ~/kphp_new


echo -e "\n\nCHECK THAT www ARE COMPILED IN BRANCH ${BRANCH_NAME}\n\n" && sleep 1
git checkout $BRANCH_NAME
make -j php -C ~/engine
KPHP_BUILD_ARGS='build_args:-vvv' KPHP_CUSTOM_LIBS="kphp_custom_libs:/home/`whoami`/my-kphp-libs/all_ml/" vk mykphp || exit 1


echo -e "\n\nDIFF BETWEEN MASTER AND YOUR BRANCH\n\n" && sleep 1
diff -rubBd -I '//.*' -I '// .*' "/home/`whoami`/kphp_new" "/var/kphp/`whoami`/bin-my/kphp"

popd
