#ifndef __USER__
#define __USER__

#define MAX_USERS 100
#define USER_FILE "users.txt"

typedef struct
{
  int id;
  char username[50];
  char password[50];
  int status; // 0 = logged out, 1 = logged in
  int role;   // 0 = regular user, 1 = admin
} User;

/* Function prototypes */
void load_users_from_file();
void save_user_to_file(const User *user);
int register_user(const char *username, const char *password);
int login_user(const char *username, const char *password, int *user_id);
void logout_user(const char *username);
int is_admin(const char *username);

#endif
