###########
# project #
###########
project('ghostmirror', 'c', version: '0.9.22')
cc = meson.get_compiler('c')
localPath = meson.current_source_dir()

type = 'executable'

version = {
  'mmr': meson.project_version().split('.'),
  'str': meson.project_version(),
  'dsc': 'fix path_explode ..'
}

if get_option('eveloper')
  if run_command('grep', meson.project_name() + ' v' + version['str'], './README.md', check: false).returncode() > 0
    message('update README')
    run_command('sed', '-i', '-r', '-e', 's/^' + meson.project_name() + '[ \\t]*v(.*)/' + meson.project_name() + ' v' + version['str'] + '/', './README.md', check: true)
    run_command('sed', '-i', '-r', '-e', '0,/^\* v.*/{s/\*(.*)/\* v' + version['str'] + ' ' + version['dsc'] + '\\n\*\\1/}' , './README.md', check: true).stdout().strip()
    message('update PKGBUILD')
    run_command('sed', '-i', '-r', '-e', 's/^pkgver=\'([^\']+)\'/pkgver=\'' + version['str'] + '\'/', './distro/PKGBUILD', check: true)
    message('generating MANPAGES')
    run_command('bash', './man/gen-manpages.sh', env: {'CURRENT_VERSION': version['str']}, check: true)
    message('update GIT')
    run_command('git', 'commit', '-a', '-m', version['dsc'], check: true)
    run_command('git', 'tag', '-a', 'v' + version['str'], '-m', version['dsc'], check: true)
    run_command('git', 'push', '--follow-tags', check: true)
    message('update AUR')
    run_command('cp', './distro/PKGBUILD', './distro/ghostmirror/PKGBUILD', check: true)
    run_command('bash', '-c', 'makepkg -D ./distro --printsrcinfo > ./distro/ghostmirror/.SRCINFO', check: true)
    run_command('git', '-C', './distro/ghostmirror', 'commit', '-a', '-m', version['dsc'], check: true)
    run_command('git', '-C', './distro/ghostmirror', 'push', check: true)
  endif
endif
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
src += [ 'notstd/opt.c' ]
src += [ 'notstd/delay.c' ]

src += [ 'src/www.c' ]
src += [ 'src/archive.c' ]
src += [ 'src/gm.c' ]
src += [ 'src/arch.c' ]
src += [ 'src/investigation.c' ]
src += [ 'src/systemd.c' ]

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
add_global_arguments('-Wall', language: 'c')
add_global_arguments('-Wextra', language: 'c')
add_global_arguments('-Wuninitialized', language: 'c')
add_global_arguments('-Winit-self', language: 'c')
add_global_arguments('-Wstrict-aliasing', language: 'c')
add_global_arguments('-Wstrict-overflow', language: 'c')
add_global_arguments('-Wfloat-equal', language: 'c')
add_global_arguments('-Wvla', language: 'c')

# unicode var name
#add_global_arguments('-finput-charset=UTF-8', language: 'c')
add_global_arguments('-fextended-identifiers', language: 'c')

#reentrant malloc
add_global_arguments('-pthread', language: 'c')
add_global_link_arguments('-pthread', language:'c')

# standard
add_global_arguments('-std=gnu2x', language: 'c')

# open mp
if get_option('openmp') > 0
  message('openmp enabled')
  add_global_arguments('-fopenmp', language:'c')
  add_global_link_arguments('-fopenmp', language:'c')
  add_global_arguments('-DOMP_ENABLE=1', language: 'c')
endif 

# gprof
if get_option('gprof') > 0
  add_global_arguments('-pg', language:'c')
  add_global_link_arguments('-pg', language:'c')
endif

# optimization
if( get_option('optimize') == 3 )
  message('agressive optimization enabled')
  add_global_arguments('-O3', language: 'c')
  add_global_arguments('-march=native', language: 'c')
  add_global_arguments('-mtune=native', language: 'c')
elif( get_option('optimize') == 2 )
  message('local optimization enabled')
  add_global_arguments('-O2', language: 'c')
  add_global_arguments('-march=native', language: 'c')
  add_global_arguments('-mtune=native', language: 'c')
elif( get_option('optimize') == 1 )
  message('generic optimization enabled')
  add_global_arguments('-O2', language: 'c')
else
  add_global_arguments('-O0', language: 'c') 
  add_global_arguments('-g', language: 'c')
endif

if get_option('autovectorization') > 0
  if( get_option('optimize') < 1 )
	error('required option optimize > 0')
  endif
  message('vectorization enabled')
  add_global_arguments('-ftree-vectorize', language:'c')
  add_global_arguments('-DVECTORIZE=1', language:'c')
endif

##########
# Define #
##########

add_global_arguments('-DVERSION_STR="' + version['str'] + '"', language: 'c')
add_global_arguments('-DVERSION_MAJ="' + version['mmr'][0] + '"', language: 'c')
add_global_arguments('-DVERSION_MIN="' + version['mmr'][1] + '"', language: 'c')
add_global_arguments('-DVERSION_REV="' + version['mmr'][2] + '"', language: 'c')

##########################
# libraries dependencies #
##########################
libDeps  = [ cc.find_library('m', required : true) ] # math
libDeps += [ dependency('libcurl', required: true) ]
libDeps += [ dependency('zlib', required: true) ]
libDeps += [ dependency('libsystemd', required: true) ]

#########################
# software dependencies #
#########################

#################
# Custom Target #
#################

#########
# debug #
#########

opt_debug = get_option('ebug')
if opt_debug > 0
  message('debug enabled @0@'.format(opt_debug))
  add_global_arguments('-DDBG_ENABLE=@0@'.format(opt_debug), language: 'c')
  #add_global_arguments('-ftree-vectorizer-verbose=5', language:'c')
  #add_global_arguments('-fopt-info-loop-optimized', language:'c')
  #add_global_arguments('-fopt-info-vec-optimized', language:'c')
  #add_global_arguments('-fopt-info-vec-missed', language:'c')
endif

if get_option('olor') > 0
  message('color debug enabled')
  add_global_arguments('-DDBG_COLOR=1', language: 'c')
endif

if get_option('shortpath') > 0
  message('shortpath debug enabled')
  add_global_arguments('-DDBG_SHORTPATH=1', language: 'c')
endif

if get_option('shortfn') > 0
  message('shortfn debug enabled')
  add_global_arguments('-DDBG_SHORTFN=1', language: 'c')
endif

if get_option('assert')
  message('assertion enabled')
  add_global_arguments('-DASSERT_ENABLE=1', language: 'c')
endif

###########
# license #
###########

message('copyright vbextreme 2024')
message('released under GPLv3')

#########
# build #
#########

if type == 'executable' 
  executable(meson.project_name(), src, include_directories: includeDir, dependencies: libDeps, install: true)
else
  shared_library(meson.project_name(), src, include_directories: includeDir, dependencies: libDeps, install: true)
endif















