#include "common.h"

#define USER_DIR "./users/"

typedef struct
{
    char username[USERNAME_MAX_LEN];
    char password[1024];
    char biography[1024];
} User;

int load_user(const char *username, User *user);
int save_user(const User *user);