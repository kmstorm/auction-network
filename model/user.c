#include "user.h"
#include <stdio.h>
#include <string.h>

/* Global variables */
static User users[MAX_USERS];
static int user_count = 0;

/* Load users from file */
void load_users_from_file()
{
  FILE *file = fopen(USER_FILE, "r");
  if (!file)
  {
    printf("No user file found, starting fresh.\n");
    return;
  }

  while (fscanf(file, "%49s %49s %d %d\n", users[user_count].username, users[user_count].password, &users[user_count].status, &users[user_count].role) != EOF)
  {
    user_count++;
  }

  fclose(file);
}

/* Save a user to the file */
void save_user_to_file(const User *user)
{
  FILE *file = fopen(USER_FILE, "a");
  if (!file)
  {
    perror("Failed to open user file");
    return;
  }

  fprintf(file, "%s %s %d %d\n", user->username, user->password, user->status, user->role);
  fclose(file);
}

/* Register a new user */
int register_user(const char *username, const char *password)
{
  for (int i = 0; i < user_count; i++)
  {
    if (strcmp(users[i].username, username) == 0)
    {
      return 0; // User already exists
    }
  }

  if (user_count < MAX_USERS)
  {
    strcpy(users[user_count].username, username);
    strcpy(users[user_count].password, password);
    users[user_count].status = 0; // Initially logged out
    users[user_count].role = 0;   // Default role is user (0)
    save_user_to_file(&users[user_count]);
    user_count++;
    return 1;
  }

  return 0;
}

/* Login user */
int login_user(const char *username, const char *password)
{
  for (int i = 0; i < user_count; i++)
  {
    if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0)
    {
      users[i].status = 1; // Mark as logged in
      return 1;
    }
  }
  return 0;
}

/* Logout user */
void logout_user(const char *username)
{
  for (int i = 0; i < user_count; i++)
  {
    if (strcmp(users[i].username, username) == 0)
    {
      users[i].status = 0; // Mark as logged out
      break;
    }
  }
}

int is_admin(const char *username)
{
  for (int i = 0; i < user_count; i++)
  {
    if (strcmp(users[i].username, username) == 0)
    {
      return users[i].role == 1; // 1 means admin
    }
  }
  return 0; // Not found or not an admin
}
