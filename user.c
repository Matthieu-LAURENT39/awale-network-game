#include "user.h"
#include <sys/stat.h>

int load_user(const char *username, User *user)
{
    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s%s.dat", USER_DIR, username);

    FILE *fp = fopen(filepath, "r");
    if (!fp)
        return 0; // User does not exist

    strcpy(user->username, username);

    if (fgets(user->password, sizeof(user->password), fp) == NULL)
    {
        fclose(fp);
        return -1; // Error
    }
    // Remove any trailing newline
    user->password[strcspn(user->password, "\n")] = '\0';

    if (fgets(user->biography, sizeof(user->biography), fp) == NULL)
    {
        fclose(fp);
        return -1; // Error
    }
    user->biography[strcspn(user->biography, "\n")] = '\0';

    fclose(fp);
    return 1; // Success
}

int save_user(const User *user)
{
    if (access(USER_DIR, F_OK) != 0)
    {
        mkdir(USER_DIR, 0755);
    }

    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s%s.dat", USER_DIR, user->username);

    FILE *fp = fopen(filepath, "w");
    if (!fp)
    {
        perror("Failed to open user file for writing");
        return -1;
    }

    fprintf(fp, "%s\n%s\n", user->password, user->biography);
    fclose(fp);
    return 1;
}

int user_exists(const char *username)
{
    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s%s.dat", USER_DIR, username);

    FILE *fp = fopen(filepath, "r");
    if (!fp)
        return 0; // User does not exist

    fclose(fp);
    return 1; // User exists
}