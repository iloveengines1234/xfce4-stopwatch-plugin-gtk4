# Existing scalable icon installation
install_data(
  'scalable' / 'xfce4-stopwatch-plugin.svg',
  install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps',
)

# Add symbolic icon installation for modern theme compatibility
install_data(
  'symbolic' / 'xfce4-stopwatch-plugin-symbolic.svg',
  install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / 'symbolic' / 'apps',
  required: false # Keeps the build from breaking if you haven't made a symbolic asset yet
)