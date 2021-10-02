call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\vcvars32.bat"
cl sapi4.cpp ole32.lib user32.lib /MT /LD -Ox -I"C:\Program Files (x86)\Microsoft Speech SDK\Include"
cl sapi4out.cpp ole32.lib user32.lib sapi4.lib /MT -Ox -I"C:\Program Files (x86)\Microsoft Speech SDK\Include"
cl sapi4limits.cpp ole32.lib user32.lib sapi4.lib /MT -Ox -I"C:\Program Files (x86)\Microsoft Speech SDK\Include"