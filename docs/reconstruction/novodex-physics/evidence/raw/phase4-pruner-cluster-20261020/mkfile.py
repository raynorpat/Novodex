import sys,importlib.util,re
cfgpath,outtxt,dest,headerpath=sys.argv[1:5]
spec=importlib.util.spec_from_file_location('cfg',cfgpath); cfg=importlib.util.module_from_spec(spec); spec.loader.exec_module(cfg)
body=open(outtxt).read()
protos=''
for sym,lo,hi,tag,decl in cfg.ROWS:
    protos+=decl.replace('__declspec(naked) ','')+';\n'
head=open(headerpath).read()
open(dest,'w',newline='\r\n').write(head.replace('@@PROTOS@@',protos).replace('@@BODY@@',body))
