char *check_oemid(char *arg, int *argc, char **argv);
char *check_fsid(char *arg, int *argc, char **argv);
char *check_bootlabel(char *arg, int *argc, char **argv);
char *check_dirlabel(char *arg, int *argc, char **argv);
char *check_label(char *arg, int *argc, char **argv);

void check_labelargs(int *argc, char **argv,
		char **oemid, char **fsid, char **bootlabel, char **dirlabel);


dsk_err_t get_labels(DSK_PDRIVER dsk, DSK_GEOMETRY *geom, char *oemid, 
				char *fsid, char *bootlabel, char *dirlabel);

dsk_err_t set_labels(DSK_PDRIVER dsk, DSK_GEOMETRY *geom, const char *oemid, 
		const char *fsid, const char *bootlabel, const char *dirlabel);

