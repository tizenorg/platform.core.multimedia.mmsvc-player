#!/usr/bin/env python
import sys, getopt, os.path

def usage(ret):
	print "NAME"
	print "\tAuto-gen muse daemon API"
	print "\nSYNOPSIS"
	print "\tpython make_api.py -l LISTFILE [OPTION]..."
	print "\nDESCRIPTION"
	print "\t-l"
	print "\t\tLISTFILE is api list. mandatory option. make enum, dispatcher and header based this file"
	print "\t-p, --product"
	print "\t\tuse for product API"
	print "\t-s SOURCEFILE, --server SOURCEFILE"
	print "\t\tmake body template for mused module(server). must use with -s option"
	print "\t-c SOURCEFILE, --client SOURCEFILE"
	print "\t\tmake body template for capi(client). must use with -c option"
	print "\nEXAMPLE"
	print "\tpython make_api.py -l api.list"
	print "\t\tbuild time script"
	print "\tpython make_api.py -l api.list -s src/muse_player.c -c ../../api/player/src/player.c"
	print "\t\tmake body template"

	sys.exit(ret)

def make_body(ListFileName, ServerFileName, ClientFileName):
	if not (os.path.isfile(ListFileName) and os.path.isfile(ServerFileName) and os.path.isfile(ClientFileName)):
		print("File is not foundi!!\n")
		usage(1)

	print("Input new API")
	if sys.version_info >= (3,0):
		ApiName = input("player_")
	else:
		ApiName = raw_input("player_")
	ApiName = ApiName.strip()

	with open(ListFileName, "r") as ListFile:
		ApiList = ListFile.readlines()
		for Api in ApiList:
			if Api.strip() == ApiName:
				print("API already exist\n")
				sys.exit(1)

	with open(ListFileName, "a") as ListFile:
		ListFile.write(ApiName + '\n')


	with open(ServerFileName, "a") as ServerFile:
		ServerFile.write("\n")
		ServerFile.write("int player_disp_" + ApiName + "(muse_module_h module) /* Template Guide: modify parameters. */\n")
		ServerFile.write("{\n")
		ServerFile.write("\tint ret = -1;\n")
		ServerFile.write("\tintptr_t handle;\n")
		ServerFile.write("\tmuse_player_api_e api = MUSE_PLAYER_API_" + ApiName.upper() +";\n")
		ServerFile.write("\n")
		ServerFile.write("\thandle = muse_core_ipc_get_handle(module);\n")
		ServerFile.write("\n")
		ServerFile.write("\tret = ACTUAL_OPERATION_FUNCTION((player_h)handle); /* Template Guide: add legacy_player_function with parames. */\n")
		ServerFile.write("\n")
		ServerFile.write("\tplayer_msg_return(api, ret, module); /* Template Guide: call right return function with values */\n")
		ServerFile.write("\n")
		ServerFile.write("\treturn ret;\n")
		ServerFile.write("}\n")

	with open(ClientFileName, "a") as ClientFile:
		ClientFile.write("\n")
		ClientFile.write("int player_" + ApiName + "(player_h player) /* Template Guide: modify parameters. */\n")
		ClientFile.write("{\n")
		ClientFile.write("\t/* Template Guide: add checking about parameter validation. */\n")
		ClientFile.write("\tPLAYER_INSTANCE_CHECK(player);\n")
		ClientFile.write("\n")
		ClientFile.write("\tint ret = PLAYER_ERROR_NONE;\n")
		ClientFile.write("\tmuse_player_api_e api = MUSE_PLAYER_API_" + ApiName.upper() +";\n")
		ClientFile.write("\tplayer_cli_s *pc = (player_cli_s *)player;\n")
		ClientFile.write("\n")
		ClientFile.write("\tchar *ret_buf = NULL;\n")
		ClientFile.write("\n")
		ClientFile.write("\t/* Template Guide: add needed implementation and call send function with values. */\n")
		ClientFile.write("\t/* ... */\n")
		ClientFile.write("\tplayer_msg_send(api, pc, ret_buf, ret);\n")
		ClientFile.write("\n")
		ClientFile.write("\tg_free(ret_buf);\n")
		ClientFile.write("\n")
		ClientFile.write("\treturn ret;\n")
		ClientFile.write("}\n")

def build_running(ListFileName, product):

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
		line = line.strip()
		if len(line) > 1:
			DefFile.write("MUSE_PLAYER_API_" + line.upper() +",\n")
			FuncFile.write("player_disp_" + line + ",\n")
			HeadFile.write("int player_disp_" + line + "(muse_module_h module);\n")

	HeadFile.write(HeaderPostfix)

	DefFile.close()
	ListFile.close()
	FuncFile.close()
	HeadFile.close()

def main(argv):
	product = ""
	make_body_opt = 0
	ServerFileName = ""
	ClientFileName = ""
	ListFileName = ""
	try:
		opts, args = getopt.getopt(argv, "hpl:s:c:", ["product","list","server","client"])
	except getopt.GetoptError:
		usage(1)
	for opt, arg in opts:
		if opt == '-h':
			usage(0)
		elif opt in ("-l", "--list"):
			ListFileName = arg
		elif opt in ("-p", "--product"):
			product = "product_"
		elif opt in ("-s", "--server"):
			make_body_opt = make_body_opt + 1
			ServerFileName = arg
		elif opt in ("-c", "--client"):
			make_body_opt = make_body_opt + 1
			ClientFileName = arg

	if not ListFileName:
		usage(1)
	elif make_body_opt >= 2:
		make_body(ListFileName, ServerFileName, ClientFileName)
	elif make_body_opt:
		usage(1)
	else:
		build_running(ListFileName, product)

if __name__ == "__main__":
	main(sys.argv[1:])
