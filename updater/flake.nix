{
  description = "Nix flake for building and installing the orbbec SDK";

  inputs = {
    orbbecSDK.url =
      "git+ssh://git@github.com/adrian-kira/OrbbecSDK.git?ref=feature/nix";
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs, ... }@inputs:
    let
      orbbecSDK = inputs.orbbecSDK.packages.x86_64-linux.default;
      pkgs = nixpkgs.legacyPackages.x86_64-linux;
      orbbecFWUpdater = pkgs.stdenv.mkDerivation {
        pname = "orbbec-fw-updater";
        version = "1.0";
        src = ./.;

        nativeBuildInputs = [ pkgs.cmake pkgs.pkg-config ];

        buildInputs = [ orbbecSDK pkgs.boost ];

        cmakeFlags = [ ];

        meta = with pkgs.lib; {
          description = "Orbbec Firmware Updater";
          homepage = "https://github.com/adrian-kira/OrbbecFirmwareUpdater";
          license = licenses.mit;
          maintainers = [ ];
          platforms = platforms.linux;
          mainProgram = "OrbbecFirmwareUpdater";
        };
      };
    in { packages.x86_64-linux.default = orbbecFWUpdater; };
}
