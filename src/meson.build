rofi = dependency('rofi', version: '>= 1.7.0')
shared_module(
  'date',
  ['date.c'],
  dependencies: [ rofi],
  install: true,
  install_dir: rofi.get_variable('pluginsdir'),
)
