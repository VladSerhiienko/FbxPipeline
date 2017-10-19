#!/usr/bin/env python

top = '.'
out = 'build'


def check_compiler_flag(conf, flag, lang):
	return conf.check(fragment = 'int main() { float f = 4.0; char c = f; return c - 4; }\n', execute = 0, define_ret = 0, msg = 'Checking for compiler switch %s' % flag, cxxflags = conf.env[lang + 'FLAGS'] + [flag], okmsg = 'yes', errmsg = 'no')  # the code inside fragment deliberately does an unsafe implicit cast float->char to trigger a compiler warning; sometimes, gcc does not tell about an unsupported parameter *unless* the code being compiled causes a warning

def add_compiler_flags(conf, env, flags, lang, uselib = ''):
	for flag in flags:
		if type(flag) == type(()):
			flag_candidate = flag[0]
			flag_alternative = flag[1]
		else:
			flag_candidate = flag
			flag_alternative = None

		if uselib:
			flags_pattern = lang + 'FLAGS_' + uselib
		else:
			flags_pattern = lang + 'FLAGS'

		if check_compiler_flag(conf, flag_candidate, lang):
			env[flags_pattern] += [flag_candidate]
		elif flag_alternative:
			if check_compiler_flag(conf, flag_alternative, lang):
				env[flags_pattern] += [flag_alternative]


def check_opengl(conf):
	if conf.check_cc(framework_name = 'OpenGL', uselib_store = 'OPENGL', mandatory=0):
		conf.define('HAVE_OSX_OPENGL', 1)
		return

	gl_h = conf.check_cc(header_name = 'GL/gl.h', uselib_store = 'OPENGL', mandatory=1)

	ret = conf.check_cc(lib = 'GL', uselib_store = 'OPENGL')
	if ret and gl_h:
		conf.define('HAVE_X11_GL', 1)
		return

	ret = conf.check_cc(lib = 'OpenGL32', uselib_store = 'OPENGL')
	if ret and gl_h:
		conf.define('HAVE_WINDOWS_GL', 1)
		return

	conf.fatal("Could not find OpenGL libraries")


def options(opt):
	opt.load('compiler_cc compiler_cxx')
	opt.load('waf_unit_test')
	opt.add_option('--debug', action='store_true', default=False, help='create debug build (default: release)')
	opt.recurse('external/ply')
	opt.recurse('external/glQuickText')
	opt.recurse('ply_viewer')


def configure(conf):
	conf.load('compiler_cc compiler_cxx')
	conf.load('waf_unit_test')

	check_opengl(conf)
	conf.check_cfg(package='glew', uselib_store='GLEW', args='--cflags --libs', mandatory=1)

	conf.recurse('external/ply')
	conf.recurse('external/glQuickText')
	conf.recurse('ply_viewer')

	opt_flags = []
	if conf.options.debug:
		opt_flags = ['-O0', '-g3', '-ggdb']
	else:
		opt_flags = ['-O2', '-fomit-frame-pointer', '-s', '-pipe']
	add_compiler_flags(conf, conf.env, opt_flags + ['-Wextra', '-Wall', ('-std=c++0x', '-std=c++98'), '-pedantic'], 'CXX')

	conf.write_config_header('config.h')


def build(bld):
	bld.recurse('external/ply')
	bld.recurse('external/glQuickText')
	bld.recurse('ply_viewer')

	import re
	r_test = re.compile('.cpp$')

	for test in bld.path.ant_glob('test/*.cpp'):
		name = test.srcpath()
		bld(
			features = ['cxx', 'cprogram', 'test'],
			target = r_test.sub('.test', name),
			use = 'ply_mesh',
			includes = '.',
			source = name
		)

	# add post-build function to show the unit test result summary
	from waflib.Tools import waf_unit_test
	bld.add_post_fun(waf_unit_test.summary)

