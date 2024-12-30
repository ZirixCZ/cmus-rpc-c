#include "discord_game_sdk.h"
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef DEBUG
static int silent_mode = 0;
#else
static int silent_mode = 1;
#endif

volatile sig_atomic_t running = 1;

struct discord_application {
  struct IDiscordCore *core;
  struct IDiscordUsers *users;
};

struct cmus_status {
  char status[32];
  char file[256];
  int duration;
  int position;
  char comment[256];
  char title[256];
  char artist[256];
  char album[256];
  char albumartist[256];
  char genre[256];
  int tracknumber;
  int discnumber;
  int totaldiscs;
  char date[32];
  int aaa_mode;
  int continue_play;
  int play_library;
  int play_sorted;
  int replaygain_disabled;
  int replaygain_limit;
  float replaygain_preamp;
  int repeat;
  int repeat_current;
  int shuffle;
  int softvol;
  int vol_left;
  int vol_right;
};

void on_ready(void *data);
void handle_sigint(int sig);
int debug_log(const char *format, ...);
void print_cmus_status(struct cmus_status *status);
int execute_cmus_remote(struct cmus_status *status);
void populate_discord_activity(struct DiscordActivity *activity,
                               struct cmus_status *status);
void populate_discord_params(struct DiscordCreateParams *params);
void on_disconnected(void *data, int code, const char *message);
void on_activity_update(void *callback_data, enum EDiscordResult result);

int main() {
  if (signal(SIGINT, handle_sigint) == SIG_ERR) {
    debug_log(
        "Unable to catch SIGINT, unexpected behaviour might occur (discord "
        "might need to be restarted for the activity to disappear)\n");
    return 1;
  }

  if (signal(SIGTERM, handle_sigint) == SIG_ERR) {
    debug_log(
        "Unable to catch SIGTERM. Unexpected behaviour might occur (discord "
        "might need to be restarted for the activity to disappear)\n");
    return 1;
  }

  struct discord_application app;
  memset(&app, 0, sizeof(app));

  struct DiscordCreateParams params;
  populate_discord_params(&params);

  enum EDiscordResult initialization_result =
      DiscordCreate(DISCORD_VERSION, &params, &app.core);

  if (initialization_result != DiscordResult_Ok) {
    debug_log("Failed to initialize Discord SDK: %d\n", initialization_result);
  }
  debug_log("Discord SDK initialized successfully.\n");

  struct IDiscordActivityManager *activity_manager =
      app.core->get_activity_manager(app.core);

  if (!activity_manager) {
    debug_log("Failed to obtain Activity Manager.\n");
    app.core->destroy(app.core);
    return 1;
  }
  debug_log("Activity Manager obtained successfully.\n");

  struct cmus_status status;
  memset(&status, 0, sizeof(status));

  struct DiscordActivity activity;
  memset(&activity, 0, sizeof(activity));

  while (running) {
    app.core->run_callbacks(app.core);

    execute_cmus_remote(&status);
    if (!silent_mode) {
      print_cmus_status(&status);
    }
    populate_discord_activity(&activity, &status);
    activity_manager->update_activity(activity_manager, &activity, NULL,
                                      on_activity_update);

    usleep(1000000);
  }

  debug_log("Initiating shutdown, clearing activity...\n");

  if (app.core) {
    app.core->destroy(app.core);
    app.core = NULL;
    debug_log("Discord Core destroyed successfully.\n");
  }

  debug_log("Application exited gracefully.\n");
  return 0;
}

void on_ready(void *data) { printf("Discord SDK is ready!\n"); }

void on_disconnected(void *data, int code, const char *message) {
  debug_log("Discord SDK disconnected: %s (Code: %d)\n", message, code);
}

void on_activity_update(void *callback_data, enum EDiscordResult result) {
  if (result == DiscordResult_Ok) {
    debug_log("Activity updated successfully.\n");
  } else {
    debug_log("Failed to update activity: %d\n", result);
  }
}

/* You are not limited by this selection. Feel free to change/extend it.
 * DiscordActivity documentation:
 * https://discord.com/developers/docs/events/gateway-events#activity-object
 */
void populate_discord_activity(struct DiscordActivity *activity,
                               struct cmus_status *status) {
  strncpy(activity->name, "Playing Solo", sizeof(activity->name) - 1);
  activity->type = DiscordActivityType_Listening;
  strncpy(activity->details, status->title, sizeof(activity->details) - 1);
  strncpy(activity->state, status->artist, sizeof(activity->state) - 1);
  strncpy(activity->assets.large_image, "https://i.imgur.com/4Y4hcwv.jpeg",
          sizeof(activity->assets.large_image) - 1);
  strncpy(activity->assets.large_text, "CMUS",
          sizeof(activity->assets.large_text) - 1);
  activity->instance = false;
}

void populate_discord_params(struct DiscordCreateParams *params) {
  // You can obtain your client ID by creating a new application on Discord's
  // Developer Portal: https://discord.com/developers/applications
  // The application name will be displayed as the text right next to "Playing"
  params->client_id = 1323030031068561469;
  params->flags = DiscordCreateFlags_Default;
  params->events = NULL;
  params->event_data = NULL;
}

int debug_log(const char *format, ...) {
  if (silent_mode) {
    return 0;
  }

  va_list args;
  va_start(args, format);
  int result = vprintf(format, args);
  va_end(args);

  return result;
}

void handle_sigint(int sig) { running = 0; }

int execute_cmus_remote(struct cmus_status *status) {
  FILE *fp = popen("cmus-remote -Q", "r");
  if (fp == NULL) {
    perror("popen failed");
    return -1;
  }

  char line[256];

  while (fgets(line, sizeof(line), fp) != NULL) {
    if (strncmp(line, "status", 6) == 0) {
      sscanf(line, "status %s", status->status);
    } else if (strncmp(line, "file", 4) == 0) {
      sscanf(line, "file %255[^\n]", status->file);
    } else if (strncmp(line, "duration", 8) == 0) {
      sscanf(line, "duration %d", &status->duration);
    } else if (strncmp(line, "position", 8) == 0) {
      sscanf(line, "position %d", &status->position);
    } else if (strncmp(line, "tag comment", 11) == 0) {
      sscanf(line, "tag comment %255[^\n]", status->comment);
    } else if (strncmp(line, "tag title", 9) == 0) {
      sscanf(line, "tag title %255[^\n]", status->title);
    } else if (strncmp(line, "tag artist", 10) == 0) {
      sscanf(line, "tag artist %255[^\n]", status->artist);
    } else if (strncmp(line, "tag album", 9) == 0) {
      sscanf(line, "tag album %255[^\n]", status->album);
    } else if (strncmp(line, "tag albumartist", 15) == 0) {
      sscanf(line, "tag albumartist %255[^\n]", status->albumartist);
    } else if (strncmp(line, "tag genre", 9) == 0) {
      sscanf(line, "tag genre %255[^\n]", status->genre);
    } else if (strncmp(line, "tag tracknumber", 15) == 0) {
      sscanf(line, "tag tracknumber %d", &status->tracknumber);
    } else if (strncmp(line, "tag discnumber", 14) == 0) {
      sscanf(line, "tag discnumber %d", &status->discnumber);
    } else if (strncmp(line, "tag totaldiscs", 14) == 0) {
      sscanf(line, "tag totaldiscs %d", &status->totaldiscs);
    } else if (strncmp(line, "tag date", 8) == 0) {
      sscanf(line, "tag date %31[^\n]", status->date);
    } else if (strncmp(line, "set aaa_mode", 12) == 0) {
      sscanf(line, "set aaa_mode %d", &status->aaa_mode);
    } else if (strncmp(line, "set continue", 12) == 0) {
      sscanf(line, "set continue %d", &status->continue_play);
    } else if (strncmp(line, "set play_library", 16) == 0) {
      sscanf(line, "set play_library %d", &status->play_library);
    } else if (strncmp(line, "set play_sorted", 15) == 0) {
      sscanf(line, "set play_sorted %d", &status->play_sorted);
    } else if (strncmp(line, "set replaygain", 14) == 0) {
      if (strstr(line, "disabled")) {
        status->replaygain_disabled = 1;
      } else {
        status->replaygain_disabled = 0;
      }
    } else if (strncmp(line, "set replaygain_limit", 20) == 0) {
      sscanf(line, "set replaygain_limit %d", &status->replaygain_limit);
    } else if (strncmp(line, "set replaygain_preamp", 21) == 0) {
      sscanf(line, "set replaygain_preamp %f", &status->replaygain_preamp);
    } else if (strncmp(line, "set repeat", 10) == 0) {
      sscanf(line, "set repeat %d", &status->repeat);
    } else if (strncmp(line, "set repeat_current", 18) == 0) {
      sscanf(line, "set repeat_current %d", &status->repeat_current);
    } else if (strncmp(line, "set shuffle", 11) == 0) {
      sscanf(line, "set shuffle %d", &status->shuffle);
    } else if (strncmp(line, "set softvol", 11) == 0) {
      sscanf(line, "set softvol %d", &status->softvol);
    } else if (strncmp(line, "set vol_left", 12) == 0) {
      sscanf(line, "set vol_left %d", &status->vol_left);
    } else if (strncmp(line, "set vol_right", 13) == 0) {
      sscanf(line, "set vol_right %d", &status->vol_right);
    }
  }

  pclose(fp);

  return 0;
}

void print_cmus_status(struct cmus_status *status) {
  debug_log("\n\n\n");
  debug_log("Status: %s\n", status->status);
  debug_log("File: %s\n", status->file);
  debug_log("Duration: %d\n", status->duration);
  debug_log("Position: %d\n", status->position);
  debug_log("Title: %s\n", status->title);
  debug_log("Artist: %s\n", status->artist);
  debug_log("Album: %s\n", status->album);
  debug_log("Track Number: %d\n", status->tracknumber);
  debug_log("Replaygain Preamp: %.2f\n", status->replaygain_preamp);
  debug_log("\n\n\n");
}
