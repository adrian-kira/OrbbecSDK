{
  description = "Nix flake for building and installing the orbbec SDK";

  inputs = { nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable"; };

  outputs = { self, nixpkgs }:
    let
      pkgs = nixpkgs.legacyPackages.x86_64-linux;
      orbbecSDK = pkgs.stdenv.mkDerivation {
        name = "orbbec-sdk";
        src = ./.;

        nativeBuildInputs = [ pkgs.cmake ];

        buildInputs = [ ];

        cmakeFlags = [ ];

        #        installPhase = ''
        #          # Use CMake's install target instead of manual copying
        #          cmake --build . --target install --config Release
        #        '';
      };
    in { packages.x86_64-linux.default = orbbecSDK; };
}
