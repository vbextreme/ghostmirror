###########
# project #
###########

#--buildtype=plain

project('ghostmirror', 'c', version: '0.18.4', meson_version: '>= 1.1')

cc = meson.get_compiler('c')
localPath = meson.current_source_dir()

version = {
  'mmr': meson.project_version().split('.'),
  'str': meson.project_version(),
  'dsc': 'speed fail; meson.developer bug need to solve this'
}

if get_option('eveloper')
  custom_target('xrelease',
    build_by_default: false,
    build_always_stale: true,
    command: [
      'meson.developer',
      '-d', 'curl',
      '-d', 'zlib-ng',
      '-d', 'systemd-libs',
      '-d', 'elfutils',
      '-D', version['dsc'],
    ],
    output: 'xrelease'
  )
endif

##############
# subproject #
##############

happycrash = subproject('happycrash')

###########
# include #
###########

includeDir  = include_directories('include')

###############
# source file #
###############

src  = [ 'notstd/core.c' ]
src += [ 'notstd/err.c' ]
src += [ 'notstd/math.c' ]
src += [ 'notstd/memory.c' ]
src += [ 'notstd/extras.c' ]
src += [ 'notstd/futex.c' ]
src += [ 'notstd/str.c' ]
src += [ 'notstd/utf8.c' ]
src += [ 'notstd/opt.c' ]
src += [ 'notstd/delay.c' ]

src += [ 'src/inutility.c' ]
src += [ 'src/www.c' ]
src += [ 'src/ping.c' ]
src += [ 'src/archive.c' ]
src += [ 'src/gm.c' ]
src += [ 'src/arch.c' ]
src += [ 'src/mirrorlist.c' ]
src += [ 'src/investigation.c' ]
src += [ 'src/systemd.c' ]

##########################
# libraries dependencies #
##########################
libDeps  = [ cc.find_library('m', required : true) ] # math
libDeps += [ dependency('libcurl', required: true) ]
libDeps += [ dependency('zlib-ng', required: true) ]
libDeps += [ dependency('libsystemd', required: true) ]
libDeps += [ dependency('libelf', required: true) ]
libDeps += [ dependency('libhappycrash', fallback: 'libhappycrash', required: true) ]

##############
# data files #
##############

#############
# man files #
#############

install_man('man/ghostmirror.1')

#########################
# bash completion files #
#########################

install_data('bash-completion/ghostmirror', install_dir: '/etc/bash_completion.d/')

##################
# compiler flags #
##################

# warnings
add_project_arguments('-Wall', language: 'c')
add_project_arguments('-Wextra', language: 'c')
add_project_arguments('-Wuninitialized', language: 'c')
add_project_arguments('-Winit-self', language: 'c')
add_project_arguments('-fstrict-aliasing', language: 'c')
add_project_arguments('-Wstrict-aliasing=2', language: 'c')
add_project_arguments('-Wstrict-overflow', language: 'c')
add_project_arguments('-Wfloat-equal', language: 'c')
add_project_arguments('-Wvla', language: 'c')
add_project_arguments('-fextended-identifiers', language: 'c')
add_project_arguments('-pthread', language: 'c')
add_project_arguments('-std=gnu2x', language: 'c')

add_project_arguments(happycrash.get_variable('project_arguments'), language: 'c')
add_project_arguments('-DHAPPYCRASH_ENABLE', language: 'c')

add_project_link_arguments('-pthread', language:'c')

# open mp
if get_option('openmp')
  message('openmp enabled')
  add_project_arguments('-fopenmp', language:'c')
  add_project_link_arguments('-fopenmp', language:'c')
  add_project_arguments('-DOMP_ENABLE=1', language: 'c')
endif 

# gprof
if get_option('gprof')
  add_project_arguments('-pg', language:'c')
  add_project_link_arguments('-pg', language:'c')
endif


# optimization
optimize = get_option('optimize')
if( optimize == 'heavy' )
  message('agressive optimization enabled')
  add_project_arguments('-O2', language: 'c')
  add_project_arguments('-march=native', language: 'c')
  add_project_arguments('-mtune=native', language: 'c')
  if( get_option('autovectorization') )
    message('vectorization enabled')
    add_project_arguments('-ftree-vectorize', language:'c')
    add_project_arguments('-DVECTORIZE=1', language:'c')
  endif
elif( optimize == 'normal' )
  message('normal optimization enabled')
  add_project_arguments('-O2', language: 'c')
elif( optimize == 'debug' )
  message('debug optimization enabled')
  add_project_arguments('-g', language: 'c')
  add_project_arguments('-O1', language: 'c') 
  add_project_arguments('-fsanitize=address', language: 'c')
  add_project_link_arguments('-fsanitize=address', language: 'c')
  add_project_arguments('-fno-omit-frame-pointer', language: 'c')
  libDeps += [ cc.find_library('libasan', required : true) ]
endif

##########
# Define #
##########

add_project_arguments('-DVERSION_STR="' + version['str'] + '"', language: 'c')
add_project_arguments('-DVERSION_MAJ="' + version['mmr'][0] + '"', language: 'c')
add_project_arguments('-DVERSION_MIN="' + version['mmr'][1] + '"', language: 'c')
add_project_arguments('-DVERSION_REV="' + version['mmr'][2] + '"', language: 'c')

#########################
# software dependencies #
#########################
hc = find_program('happycrash', required: true)

#################
# Custom Target #
#################

#########
# debug #
#########

opt_debug = get_option('ebug')
if opt_debug > 0
  message('debug enabled @0@'.format(opt_debug))
  add_project_arguments('-DDBG_ENABLE=@0@'.format(opt_debug), language: 'c')
  #add_project_arguments('-ftree-vectorizer-verbose=5', language:'c')
  #add_project_arguments('-fopt-info-loop-optimized', language:'c')
  #add_project_arguments('-fopt-info-vec-optimized', language:'c')
  #add_project_arguments('-fopt-info-vec-missed', language:'c')
endif

if get_option('olor')
  message('color debug enabled')
  add_project_arguments('-DDBG_COLOR=1', language: 'c')
endif

if get_option('shortpath')
  message('shortpath debug enabled')
  add_project_arguments('-DDBG_SHORTPATH=1', language: 'c')
endif

if get_option('shortfn')
  message('shortfn debug enabled')
  add_project_arguments('-DDBG_SHORTFN=1', language: 'c')
endif

if get_option('assert')
  message('assertion enabled')
  add_project_arguments('-DASSERT_ENABLE=1', language: 'c')
endif

###########
# license #
###########

message('copyright vbextreme 2024')
message('released under GPLv3')

#########
# build #
#########

elf = executable('@0@.elf'.format(meson.project_name()), src, include_directories: includeDir, dependencies: libDeps, install: false)
custom_target('happycrash',
  build_by_default: true,
  input: elf,
  output: meson.project_name(),
  install: true,
  install_dir: get_option('bindir'),
  command: [ hc, '--version', version['str'], '-s', '-e' , '@INPUT@', '-o', '@OUTPUT@' ]
)

