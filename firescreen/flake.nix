{
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      forAllSystems = f: nixpkgs.lib.genAttrs [ "x86_64-linux" "aarch64-linux" ]
        (system: f nixpkgs.legacyPackages.${system});
    in {
      devShells = forAllSystems (pkgs: {
        default = pkgs.mkShell {
          packages = with pkgs; [
            gcc # pio test -e native, tools/preview
            imagemagick # tools/preview PGM -> PNG
            librsvg # scripts/gen_icons.py
            (python3.withPackages (p: [
              p.pillow # gen_icons.py
              p.freetype-py # gen_fonts.py
              p.pyserial # fake_data.py
            ]))
          ];

          # Default TTF source for scripts/gen_fonts.py.
          IOSEVKA_TTF_DIR = "${pkgs.iosevka}/share/fonts/truetype";
        };
      });
    };
}
