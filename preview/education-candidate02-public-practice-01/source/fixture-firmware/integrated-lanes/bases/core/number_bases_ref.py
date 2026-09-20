"""Independent strict reference for EntropyLab's four extra number bases."""
ALPH={4:"0123",8:"01234567",32:"qpzry9x8gf2tvdw0s3jn54khce6mua7l",64:"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"}
BITS={4:2,8:3,32:5,64:6}
WORD_BITS={12:128,15:160,18:192,21:224,24:256}
class NumberBaseError(ValueError): pass
def _meta(base,words):
    if base not in ALPH or words not in WORD_BITS: raise NumberBaseError("unsupported")
    bits=WORD_BITS[words]; width=BITS[base]; full=bits//width; rem=bits%width
    return bits,width,full,rem,full+(rem if base==64 else 1 if rem else 0)
def entropy_bytes(words):
    if words not in WORD_BITS: raise NumberBaseError("unsupported")
    return WORD_BITS[words]//8
def digit_count(base,words): return _meta(base,words)[4]
def encode(data,base,words):
    bits,width,full,rem,count=_meta(base,words)
    if len(data)!=bits//8: raise NumberBaseError("length")
    stream="".join(f"{x:08b}" for x in data); a=ALPH[base]
    out="".join(a[int(stream[i*width:(i+1)*width],2)] for i in range(full))
    if rem: out += stream[full*width:] if base==64 else a[int(stream[full*width:],2)]
    assert len(out)==count
    return out
def decode(text,base,words):
    bits,width,full,rem,count=_meta(base,words); a=ALPH[base]
    # Python's Unicode isspace mirrors JS \s closely, but not U+FEFF; contract tests use ASCII whitespace.
    chars=[]; invalid=False
    for c in text:
        if c.isspace() or c=='\ufeff': continue
        n=c if base==64 else c.lower() if base==32 else c.upper()
        if n not in a:
            invalid=True
            continue
        chars.append(n)
    if not chars: raise NumberBaseError("empty")
    if invalid: raise NumberBaseError("invalid character")
    if rem:
        tail=chars[full:count] if base==64 else chars[count-1:count]
        allowed="01" if base==64 else a[:1<<rem]
        if any(c not in allowed for c in tail): raise NumberBaseError("final digit")
    if len(chars)!=count: raise NumberBaseError("wrong count")
    stream=""
    for i,c in enumerate(chars):
        if base==64 and i>=full: stream+=c
        else: stream+=f"{a.index(c):0{rem if rem and i==count-1 else width}b}"
    return bytes(int(stream[i:i+8],2) for i in range(0,bits,8))
