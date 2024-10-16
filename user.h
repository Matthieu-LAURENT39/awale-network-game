#include "common.h"

#define USER_DIR "./users/"
#define MAX_FRIENDS 100

typedef struct
{
    char username[USERNAME_MAX_LEN];
    char password[1024];
    char biography[1024];
    char friends[MAX_FRIENDS][USERNAME_MAX_LEN];
} User;

int load_user(const char *username, User *user);
int save_user(const User *user);
int user_exists(const char *username);
int add_friend(const char *username, const char *friend_username);
int is_friend(const char *username, const char *friend_username);