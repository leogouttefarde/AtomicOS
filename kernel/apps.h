
#ifndef __APPS_H__
#define __APPS_H__

void init_apps();
struct uapps *get_app(const char *name);
void free_apps();

#endif
