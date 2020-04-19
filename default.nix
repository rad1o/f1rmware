with import <nixpkgs> {}; {
  env = stdenv.mkDerivation {
    name = "rad1o-f1rmware-env";
    buildInputs = [
      gcc-arm-embedded
      python2
      python2Packages.pyyaml
      gcc-arm-embedded
      vim
      perl
      git
      dfu-util
      cmake
      libusb
      cacert
    ];
  };
}
