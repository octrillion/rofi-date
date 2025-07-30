#include <glib.h>
#include <rofi/helper.h>
#include <rofi/mode-private.h>
#include <rofi/mode.h>
#include <unistd.h>

G_MODULE_EXPORT Mode mode;

typedef struct {
  gchar *message;
  gboolean message_is_error;
  GPtrArray *history;
} DatePD;

// Initialize a new private data class
static void init_Date(Mode *sw) {
  DatePD *pd = (DatePD *)mode_get_private_data(sw);
  pd->history = g_ptr_array_new();
  pd->message = "enter date here";
  pd->message_is_error = FALSE;
}

static int date_mode_init(Mode *sw) {
  if (mode_get_private_data(sw) == NULL) {
    DatePD *pd = g_malloc0(sizeof(*pd));
    mode_set_private_data(sw, pd);

    init_Date(sw);
  }
  return TRUE;
}

static unsigned int date_mode_get_num_entries(const Mode *sw) {
  DatePD *pd = (DatePD *)mode_get_private_data(sw);
  return pd->history->len;
}

static ModeMode date_mode_result(Mode *sw, int mretv, char **input,
                                 unsigned int selected_line) {
  ModeMode retv = MODE_EXIT;
  DatePD *pd = (DatePD *)mode_get_private_data(sw);
  if (mretv & MENU_CUSTOM_INPUT) {
    g_ptr_array_add(pd->history, (gpointer)g_strdup(*input));
    gchar *cmd = g_strdup_printf("/bin/sh -c \"date --date=\"%s\"\"",
                                 g_strescape(*input, NULL));
    helper_execute_command(NULL, cmd, FALSE, NULL);
    g_free(cmd);
    retv = RELOAD_DIALOG;
  }

  return retv;
}
static void date_mode_destroy(Mode *sw) {
  DatePD *pd = (DatePD *)mode_get_private_data(sw);
  if (pd != NULL) {
    g_ptr_array_free(pd->history, TRUE);
    g_free(pd);
    mode_set_private_data(sw, NULL);
  }
}

static int date_token_match(const Mode *date, rofi_int_matcher **tokens,
                            unsigned int index) {
  return FALSE;
}
static char *_get_display_value(const Mode *sw, unsigned int selected_line,
                                int *state, G_GNUC_UNUSED GList **attr_list,
                                int get_entry) {
  DatePD *pd = (DatePD *)mode_get_private_data(sw);
  return strdup(g_ptr_array_index(pd->history, selected_line));
}

extern void rofi_view_reload(void);

static char *date_mode_preprocess_input(Mode *sw, const char *input) {
  DatePD *pd = (DatePD *)mode_get_private_data(sw);

  gchar *argv[] = {"/bin/date", g_strdup_printf("-d%s", input), NULL};
  gint child_stdout;
  GPid child_pid;
  g_autoptr(GError) error = NULL;

  g_spawn_async_with_pipes(NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL,
                           NULL, &child_pid, NULL, &child_stdout, NULL, &error);
  if (error != NULL) {
    g_error("Failed to spawn child: %s", error->message);
    pd->message = strdup(error->message);
    free(error);
    return "";
  }

  char buf[1024];

  ssize_t len;
  len = read(child_stdout, buf, sizeof(buf) - 1);
  buf[len] = '\0';
  pd->message = g_strdup(buf);
  close(child_stdout);
  rofi_view_reload();
  return strdup(buf);
}

static char *date_mode_get_message(const Mode *sw) {
  DatePD *pd = (DatePD *)mode_get_private_data(sw);
  if (pd->message == NULL) {
    return "Nothing to see here :)";
  } else {
    return strdup(pd->message);
  }
}

Mode mode = {
    .abi_version = ABI_VERSION,
    .name = "date",
    .cfg_name_key = "display-date",
    ._init = date_mode_init,
    ._get_num_entries = date_mode_get_num_entries,
    ._result = date_mode_result,
    ._destroy = date_mode_destroy,
    ._token_match = date_token_match,
    ._get_display_value = _get_display_value,
    ._get_message = date_mode_get_message,
    ._get_completion = NULL,
    ._preprocess_input = date_mode_preprocess_input,
    .private_data = NULL,
    .free = NULL,
    .type = MODE_TYPE_SWITCHER,

};
