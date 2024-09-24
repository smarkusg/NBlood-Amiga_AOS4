// Generated by text2c.pl. Do not edit directly.

const char default_glbuild_fs_glsl[] =
	"#glbuild(ES2) #version 100\n"
	"#glbuild(2)   #version 110\n"
	"#glbuild(3)   #version 140\n"
	"\n"
	"#ifdef GL_ES\n"
	"#  define o_fragcolour gl_FragColor\n"
	"#elif __VERSION__ < 140\n"
	"#  define lowp\n"
	"#  define mediump\n"
	"#  define o_fragcolour gl_FragColor\n"
	"#else\n"
	"#  define varying in\n"
	"#  define texture2D texture\n"
	"out vec4 o_fragcolour;\n"
	"#endif\n"
	"\n"
	"varying mediump vec2 v_texcoord;\n"
	"\n"
	"uniform sampler2D u_palette;\n"
	"uniform sampler2D u_frame;\n"
	"\n"
	"void main(void)\n"
	"{\n"
	"  lowp float pixelvalue;\n"
	"  lowp vec3 palettevalue;\n"
	"  pixelvalue = texture2D(u_frame, v_texcoord).r;\n"
	"  palettevalue = texture2D(u_palette, vec2(pixelvalue, 0.5)).rgb;\n"
	"  o_fragcolour = vec4(palettevalue, 1.0);\n"
	"}\n"
;

const int default_glbuild_fs_glsl_size = 629;