#ifndef HUID_H_
#define HUID_H_

#define HUID_REGEXP_FOR_USER_REPORT_PURPOSES "/[0-9]{8}-[0-9]{6}/"

bool chop_huid(String_View *content, String_View *huid);
bool is_valid_huid(const char *id);
char *temp_new_huid(void);

#endif // HUID_H_
