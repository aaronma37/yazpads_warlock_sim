"""Run actual GLSL ES 3 fragment shaders in an EGL surfaceless context.
This checks shader execution, not browser/Three.js integration or user GPU speed.
"""
import ctypes as C
import ctypes.util
import json
import pathlib
import sys


def bind(lib, name, result, *args):
    fn = getattr(lib, name)
    fn.restype = result
    fn.argtypes = list(args)
    return fn


def run(manifest):
    egl = C.CDLL(ctypes.util.find_library('EGL'))
    gl = C.CDLL(ctypes.util.find_library('GLESv2'))
    pointer, integer, uint = C.c_void_p, C.c_int, C.c_uint
    address = bind(egl, 'eglGetProcAddress', pointer, C.c_char_p)(b'eglGetPlatformDisplayEXT')
    if not address:
        raise RuntimeError('EGL surfaceless display unavailable')
    display = C.CFUNCTYPE(pointer, uint, pointer, C.POINTER(integer))(address)(0x31DD, None, None)
    major, minor = integer(), integer()
    if not bind(egl, 'eglInitialize', uint, pointer, C.POINTER(integer), C.POINTER(integer))(display, C.byref(major), C.byref(minor)):
        raise RuntimeError('EGL initialization unavailable')
    bind(egl, 'eglBindAPI', uint, uint)(0x30A0)
    attributes = (integer * 11)(0x3024,8,0x3023,8,0x3022,8,0x3040,0x40,0x3033,1,0x3038)
    config, count = pointer(), integer()
    bind(egl, 'eglChooseConfig', uint, pointer, C.POINTER(integer), C.POINTER(pointer), integer, C.POINTER(integer))(display, attributes, C.byref(config), 1, C.byref(count))
    if not count.value:
        raise RuntimeError('EGL ES3 config unavailable')
    context_attributes = (integer * 3)(0x3098,3,0x3038)
    context = bind(egl, 'eglCreateContext', pointer, pointer,pointer,pointer,C.POINTER(integer))(display, config, None, context_attributes)
    if not context or not bind(egl, 'eglMakeCurrent', uint,pointer,pointer,pointer,pointer)(display,None,None,context):
        raise RuntimeError('EGL ES3 context unavailable')
    funcs = {}
    def call(name, result, types, *args):
        if name not in funcs:
            funcs[name] = bind(gl, name, result, *types)
        return funcs[name](*args)
    def shader(kind, source):
        handle = call('glCreateShader', uint,[uint],kind)
        text = C.c_char_p(source.encode())
        call('glShaderSource',None,[uint,integer,C.POINTER(C.c_char_p),pointer],handle,1,C.byref(text),None)
        call('glCompileShader',None,[uint],handle)
        status = integer()
        call('glGetShaderiv',None,[uint,uint,C.POINTER(integer)],handle,0x8B81,C.byref(status))
        if not status.value:
            log = C.create_string_buffer(16384)
            call('glGetShaderInfoLog',None,[uint,integer,pointer,pointer],handle,len(log),None,log)
            raise RuntimeError(log.value.decode())
        return handle
    def texture(width,height,data=None):
        handle = uint()
        call('glGenTextures',None,[integer,C.POINTER(uint)],1,C.byref(handle))
        call('glBindTexture',None,[uint,uint],0x0DE1,handle.value)
        for param in [0x2801,0x2800]: call('glTexParameteri',None,[uint,uint,integer],0x0DE1,param,0x2600)
        call('glTexImage2D',None,[uint,integer,integer,integer,integer,integer,uint,uint,pointer],0x0DE1,0,0x8D70,width,height,0,0x8D99,0x1405,data)
        return handle.value
    vao, buffer = uint(), uint()
    call('glGenVertexArrays',None,[integer,C.POINTER(uint)],1,C.byref(vao))
    call('glBindVertexArray',None,[uint],vao.value)
    call('glGenBuffers',None,[integer,C.POINTER(uint)],1,C.byref(buffer))
    call('glBindBuffer',None,[uint,uint],0x8892,buffer.value)
    triangle = (C.c_float * 9)(-1,-1,0,3,-1,0,-1,3,0)
    call('glBufferData',None,[uint,C.c_ssize_t,pointer,uint],0x8892,C.sizeof(triangle),triangle,0x88E4)
    outputs = []
    programs = {}
    for case in manifest['cases']:
        if case['mode'] not in programs:
            program = call('glCreateProgram',uint,[])
            for handle in [shader(0x8B31,manifest['vertex']),shader(0x8B30,manifest[case['mode']])]:
                call('glAttachShader',None,[uint,uint],program,handle)
            call('glBindAttribLocation',None,[uint,uint,C.c_char_p],program,0,b'position')
            call('glLinkProgram',None,[uint],program)
            linked = integer()
            call('glGetProgramiv',None,[uint,uint,C.POINTER(integer)],program,0x8B82,C.byref(linked))
            if not linked.value: raise RuntimeError('Shader link failed')
            programs[case['mode']] = program
        program = programs[case['mode']]
        call('glUseProgram',None,[uint],program)
        call('glEnableVertexAttribArray',None,[uint],0)
        call('glVertexAttribPointer',None,[uint,integer,uint,uint,integer,pointer],0,3,0x1406,0,0,None)
        def location(name): return call('glGetUniformLocation',integer,[uint,C.c_char_p],program,name.encode())
        for key,value in case['uniforms'].items(): call('glUniform1ui',None,[integer,uint],location(key),value)
        words = (uint * len(case['configWords']))(*case['configWords'])
        call('glUniform1uiv',None,[integer,integer,C.POINTER(uint)],location('configWords'),len(words),words)
        call('glActiveTexture',None,[uint],0x84C0)
        batch = case.get('texture', {'width':1,'height':1,'words':[0,0,0,0]})
        pixels = (uint * len(batch['words']))(*batch['words'])
        texture(batch['width'],batch['height'],pixels)
        call('glUniform1i',None,[integer,integer],location('configTex'),0)
        width,height = case['width'],case['height']
        framebuffer = uint()
        call('glGenFramebuffers',None,[integer,C.POINTER(uint)],1,C.byref(framebuffer))
        call('glBindFramebuffer',None,[uint,uint],0x8D40,framebuffer.value)
        for index in range(4):
            handle = texture(width,height)
            call('glFramebufferTexture2D',None,[uint,uint,uint,uint,integer],0x8D40,0x8CE0+index,0x0DE1,handle,0)
        if call('glCheckFramebufferStatus',uint,[uint],0x8D40) != 0x8CD5: raise RuntimeError('Integer framebuffer incomplete')
        # Attachment creation changed texture-unit zero; restore the config texture.
        config_pixels = (uint * len(batch['words']))(*batch['words'])
        texture(batch['width'],batch['height'],config_pixels)
        attachments = (uint * 4)(*[0x8CE0+i for i in range(4)])
        call('glDrawBuffers',None,[integer,C.POINTER(uint)],4,attachments)
        call('glViewport',None,[integer,integer,integer,integer],0,0,width,height)
        call('glDrawArrays',None,[uint,integer,integer],0x0004,0,3)
        report = []
        for index in range(4):
            call('glReadBuffer',None,[uint],0x8CE0+index)
            result = (uint * (width*height*4))()
            call('glReadPixels',None,[integer,integer,integer,integer,uint,uint,pointer],0,0,width,height,0x8D99,0x1405,result)
            report.append(list(result))
        error = call('glGetError',uint,[])
        if error: raise RuntimeError(f'GL error {error:#x}')
        outputs.append(report)
    renderer = call('glGetString',C.c_char_p,[uint],0x1F01).decode()
    return {'renderer':renderer,'outputs':outputs}

if __name__ == '__main__':
    try:
        print(json.dumps(run(json.loads(pathlib.Path(sys.argv[1]).read_text()))))
    except Exception as error:
        print(str(error),file=sys.stderr)
        sys.exit(1)
