AVP64_VERSION="$(cat ./AVP64_VERSION)"

echo "================================================="
echo "VP build script"
echo "================================================="
echo

echo "[*] Performing basic sanity checks..."

PLT=`uname -s`

#TODO test other distros
#if [ ! "$PLT" = "Linux" ] && [ ! "$PLT" = "Darwin" ] && [ ! "$PLT" = "FreeBSD" ] && [ ! "$PLT" = "NetBSD" ] && [ ! "$PLT" = "OpenBSD" ] && [ ! "$PLT" = "DragonFly" ]; then
if [ ! "$PLT" = "Linux" ]; then

  echo "[-] Error: VP instrumentation is unsupported on $PLT."
  exit 1

fi

test -d ./avp64/.git || echo "[*] Cloning AVP64"; git clone --recursive https://github.com/Jonaswinz/avp64 avp64

test -e ./avp64/.git || { echo "[-] avp64 not checked out, please install git or check your internet connection." ; exit 1 ; }

cd "avp64" || exit 1
echo "[*] Checking out $AVP64_VERSION"
set +e
sh -c 'git stash' 1>/dev/null 2>/dev/null
git pull 1>/dev/null 2>/dev/null
git checkout "$AVP64_VERSION" || echo Warning: could not check out to commit $AVP64_VERSION
set -e
cd "../"

echo "[+] Configuration complete."

echo "[*] Attempting to build avp64 and harness"
make -j$(nproc) || exit 1
echo "[+] Build process successful!"