{
  description = "Nix flake for building and installing the orbbec SDK with firmware updater";

  inputs = { nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable"; };

  outputs = { self, nixpkgs }:
    let
      pkgs = nixpkgs.legacyPackages.x86_64-linux;
      orbbecSDKWithUpdater = pkgs.stdenv.mkDerivation {
        name = "orbbec-sdk-with-updater";
        src = ./.;

        nativeBuildInputs = [ pkgs.cmake pkgs.pkg-config ];

        buildInputs = [ pkgs.boost ];

        cmakeFlags = [
          "-DBUILD_UPDATER=ON"
          "-DBUILD_EXAMPLES=ON"
        ];

        installPhase = ''
          mkdir -p $out
          cmake --build . --target install --config Release
          cp -r install/* $out/
        '';

        meta = with pkgs.lib; {
          description = "Orbbec SDK with firmware updater";
          homepage = "https://github.com/orbbec/OrbbecSDK";
          license = licenses.mit;
          maintainers = [ ];
          platforms = platforms.linux;
        };
      };
    in { packages.x86_64-linux.default = orbbecSDKWithUpdater; };
}
