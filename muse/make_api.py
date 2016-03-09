#!/usr/bin/env python
import sys, getopt

def main(argv):
	product = ""
	try:
		opts, args = getopt.getopt(argv, "hpl:", ["product","list"])
	except getopt.GetoptError:
		print 'make_api.py -l api.list [-p]'
		sys.exit(1)
	for opt, arg in opts:
		if opt == '-h':
			print 'make_api.py -l api.list [-p]'
			sys.exit(0)
		elif opt in ("-l", "--list"):
			ListFileName = arg
		elif opt in ("-p", "--product"):
			product = "product_"

	DefFileName = "include/muse_player_" + product + "api.def"
	FuncFileName = "include/muse_player_" + product + "api.func"
	HeadFileName = "include/muse_player_" + product + "api.h"



	WarningText = "/***************************************************************\n* This is auto-gen file from " + ListFileName + "\n* Never modify this file.\n***************************************************************/\n"
	HeaderPrefix = "#ifndef __AUTO_GEN_MUSE_PLAYER_" + product.upper() + "API_H__\n#define __AUTO_GEN_MUSE_PLAYER_" + product.upper() + "API_H__\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n"
	HeaderPostfix = "#ifdef __cplusplus\n}\n#endif\n#endif"

	ListFile = open(ListFileName)
	DefFile = open(DefFileName, "w")
	FuncFile = open(FuncFileName, "w")
	HeadFile = open(HeadFileName, "w")

	DefFile.write(WarningText)
	FuncFile.write(WarningText)
	HeadFile.write(WarningText)
	HeadFile.write(HeaderPrefix)

	for line in ListFile:
		line = line.replace('\n', '')
		if len(line) > 1:
			DefFile.write("MUSE_PLAYER_API_" + line.upper() +",\n")
			FuncFile.write("player_disp_" + line + ",\n")
			HeadFile.write("int player_disp_" + line + "(muse_module_h module);\n")

	HeadFile.write(HeaderPostfix)

	DefFile.close()
	ListFile.close()
	FuncFile.close()
	HeadFile.close()

if __name__ == "__main__":
	main(sys.argv[1:])
