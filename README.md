# uParser
A minimalistic library which implements common logic part of every byte stream parser - finding defined structure  in byte stream


# Features
- uParser foucses on lightweigth parsing byte stream, given by function uParser_streamIn;
- Parser works in byte-per-byte mode;
- Can be configured to work in two ways:
1.  by finding START and STOP synchronisation sequences;
2.  by finding delcared length in frame's header;
- To work in both ways uParser need to be correctly initialised by functions **uParser_registerByLength** or **uParser_registerByEndSequence**;
- uParser is independent due to checksum calculation, it only finds sequence and rest of bytes in the message, so can be easly linked to many binary protocols in the same way.
