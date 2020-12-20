struct ini_line
{
	int section_order;
	TCHAR *section;
	TCHAR *key;
	TCHAR *value;
};

struct ini_data
{
	struct ini_line **inidata;
	int inilines;
	bool modified;
};

struct ini_context
{
	int lastpos;
	int start;
	int end;
};

void ini_free(struct ini_data *ini);
struct ini_data *ini_new(void);
struct ini_data *ini_load(const TCHAR *path, bool sort);
bool ini_save(struct ini_data *ini, const TCHAR *path);
void ini_initcontext(struct ini_data *ini, struct ini_context *ctx);
void ini_setlast(struct ini_data *ini, const TCHAR *section, const TCHAR *key, struct ini_context *ctx);
void ini_setlastasstart(struct ini_data *ini, struct ini_context *ctx);
void ini_setnextasstart(struct ini_data *ini, struct ini_context *ctx);
void ini_setcurrentasstart(struct ini_data *ini, struct ini_context *ctx);

void ini_addnewstring(struct ini_data *ini, const TCHAR *section, const TCHAR *key, const TCHAR *val);
void ini_addnewdata(struct ini_data *ini, const TCHAR *section, const TCHAR *key, const uae_u8 *data, int len);
void ini_addnewcomment(struct ini_data *ini, const TCHAR *section, const TCHAR *val);
void ini_addnewval(struct ini_data *ini, const TCHAR *section, const TCHAR *key, uae_u32 v);
void ini_addnewval64(struct ini_data *ini, const TCHAR *section, const TCHAR *key, uae_u64 v);

bool ini_getstring(struct ini_data *ini, const TCHAR *section, const TCHAR *key, TCHAR **out);
bool ini_getstring_multi(struct ini_data *ini, const TCHAR *section, const TCHAR *key, TCHAR **out, struct ini_context*);
bool ini_getbool(struct ini_data *ini, const TCHAR *section, const TCHAR *key, bool *v);
bool ini_getval(struct ini_data *ini, const TCHAR *section, const TCHAR *key, int *v);
bool ini_getval_multi(struct ini_data *ini, const TCHAR *section, const TCHAR *key, int *v, struct ini_context*);
bool ini_getdata(struct ini_data *ini, const TCHAR *section, const TCHAR *key, uae_u8 **out, int *size);
bool ini_getdata_multi(struct ini_data *ini, const TCHAR *section, const TCHAR *key, uae_u8 **out, int *size, struct ini_context*);
bool ini_getsectionstring(struct ini_data *ini, const TCHAR *section, int idx, TCHAR **keyout, TCHAR **valout);
bool ini_getsection(struct ini_data *ini, int idx, TCHAR **section);

bool ini_addstring(struct ini_data *ini, const TCHAR *section, const TCHAR *key, const TCHAR *val);
bool ini_delete(struct ini_data *ini, const TCHAR *section, const TCHAR *key);
bool ini_nextsection(struct ini_data *ini, TCHAR *section);
