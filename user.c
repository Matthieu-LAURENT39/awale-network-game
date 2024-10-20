#include "user.h"
#include <sys/stat.h>

int load_user(const char *username, User *user)
{
    memset(user->username, 0, sizeof(user->username));
    memset(user->password, 0, sizeof(user->password));
    memset(user->biography, 0, sizeof(user->biography));
    for (int i = 0; i < MAX_FRIENDS; i++)
    {
        user->friends[i][0] = '\0';
    }

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

    // Load friends
    for (int i = 0; i < MAX_FRIENDS; i++)
    {
        if (fgets(user->friends[i], sizeof(user->friends[i]), fp) == NULL)
        {
            break;
        }
        user->friends[i][strcspn(user->friends[i], "\n")] = '\0';
    }

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
    // add friend to file
    for (int i = 0; i < MAX_FRIENDS; i++)
    {
        if (strlen(user->friends[i]) > 0)
        {
            fprintf(fp, "%s\n", user->friends[i]);
        }
    }
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

// Returns:
// 1 - Success
// 0 - Friend already exists
// -1 - Error
int add_friend(const char *username, const char *friend_username)
{
    User user;
    if (load_user(username, &user) != 1)
    {
        return -1; // Error
    }

    // Check if friend already exists
    for (int i = 0; i < MAX_FRIENDS; i++)
    {
        if (strcmp(user.friends[i], friend_username) == 0)
        {
            return 0; // Friend already exists
        }
    }

    // Find an empty slot to add friend
    for (int i = 0; i < MAX_FRIENDS; i++)
    {
        if (strlen(user.friends[i]) == 0)
        {
            strcpy(user.friends[i], friend_username);
            break;
        }
    }

    if (save_user(&user) != 1)
    {
        return -1; // Error
    }

    return 1; // Success
}

int remove_friend(const char *username, const char *friend_username)
{
    User user;
    if (load_user(username, &user) != 1)
    {
        return -1; // Error
    }

    int found = 0;
    for (int i = 0; i < MAX_FRIENDS; i++)
    {
        if (found)
        {
            // Shift friends to the left
            strcpy(user.friends[i - 1], user.friends[i]);
        }
        else if (strcmp(user.friends[i], friend_username) == 0)
        {
            user.friends[i][0] = '\0';
            found = 1;
        }
    }

    if (!found)
    {
        return 0; // Friend not found
    }

    if (save_user(&user) != 1)
    {
        return -1; // Error
    }

    return 1; // Success
}

int is_friend(const char *username, const char *friend_username)
{
    User user;
    if (load_user(username, &user) != 1)
    {
        return -1; // Error
    }

    for (int i = 0; i < MAX_FRIENDS; i++)
    {
        if (strcmp(user.friends[i], friend_username) == 0)
        {
            return 1; // Friend
        }
    }

    return 0; // Not a friend
}