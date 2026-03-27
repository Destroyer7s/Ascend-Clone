#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_STORES 30
#define MAX_PRODUCTS 50
#define MAX_QUOTES 50
#define MAX_CUSTOMERS 200
#define MAX_TRANSACTIONS 500
#define MAX_SPECIAL_ORDERS 200
#define MAX_TAX_EXCEPTIONS 200
#define MAX_TIME_CLOCK_ENTRIES 2000
#define NAME_LEN 64
#define MAX_DAYS 30

typedef enum { PAYMENT_CASH, PAYMENT_CREDIT, PAYMENT_DEBIT, PAYMENT_GIFT } PaymentType;
const char *payment_names[] = {"Cash", "Credit", "Debit", "Gift Card"};

typedef enum { SO_STATUS_NOT_ORDERED, SO_STATUS_ON_ORDER, SO_STATUS_RECEIVED, SO_STATUS_COMPLETE } SpecialOrderStatus;
const char *so_status_names[] = {"Not Yet Ordered", "On Order", "Received", "Complete"};

typedef enum { SO_TYPE_ASSOCIATE_PO, SO_TYPE_MARK_FOR_SO, SO_TYPE_SELL_NO_SO, SO_TYPE_REQUEST_TRANSFER } SpecialOrderType;

typedef enum { ACCOUNT_INDIVIDUAL, ACCOUNT_COMPANY } AccountType;

/* Customer: a person or company that purchases products and services. */
typedef struct {
    char first_name[NAME_LEN];
    char last_name[NAME_LEN];
    char company[NAME_LEN];
    AccountType account_type;
    char email[NAME_LEN];
    char phone1[NAME_LEN];
    char phone2[NAME_LEN];
    char billing_addr1[NAME_LEN];
    char billing_addr2[NAME_LEN];
    char billing_city[NAME_LEN];
    char billing_state[NAME_LEN];
    char billing_zip[NAME_LEN];
    char shipping_addr1[NAME_LEN];
    char shipping_addr2[NAME_LEN];
    char shipping_city[NAME_LEN];
    char shipping_state[NAME_LEN];
    char shipping_zip[NAME_LEN];
    int use_billing_for_shipping;
    char tax_id[NAME_LEN];
    int poa_status; // 0=inactive, 1=active, 2=suspended, 3=closed
    double credit_limit;
    int preferred_contact; // 0=email, 1=phone1, 2=phone2
    int hidden;
    char notes[NAME_LEN * 2];
} Customer;

/* Product: an item tracked in inventory with SKU and price stock information. */
typedef struct {
    char sku[NAME_LEN];      // internal stock keeping unit
    char name[NAME_LEN];     // human-readable product name
    char vendor[NAME_LEN];   // vendor name
    int serialized;          // 1=yes (serialized/bike), 0=no
    double price;            // unit retail price
    int stock;               // current available on hand in store
    int sold;                // cumulative sold count (not decremented)
} Product;

/* Quote: an estimate cart that can be converted into sale.
 * active=1 means open quote, 0 means processed.
 */
typedef struct {
    char customer[NAME_LEN];
    char quote_id[NAME_LEN];
    int item_count;
    char item_sku[MAX_PRODUCTS][NAME_LEN];
    int qty[MAX_PRODUCTS];
    double total;
    int active; // 1=open,0=closed
    int register_status[MAX_PRODUCTS]; // 0=none yet,1=registered,2=deferred,3=skipped,4=declined
    char postpone_date[MAX_PRODUCTS][NAME_LEN];
} Quote;

/* SpecialOrder: tracks out-of-stock items ordered for customers */
typedef struct {
    int special_order_id;
    int transaction_idx; // which transaction this relates to
    int store_idx; // which store
    int customer_idx; // which customer
    char product_sku[NAME_LEN];
    int qty_ordered;
    SpecialOrderStatus status;
    char order_date[NAME_LEN]; // YYYY-MM-DD
    char expected_date[NAME_LEN]; // estimated arrival
    char received_date[NAME_LEN]; // when item arrived
    char comments[NAME_LEN * 2]; // notes for buyer/staff
    int notified; // whether customer has been notified
    char notification_method[NAME_LEN]; // email, phone, text
    char po_number[NAME_LEN]; // purchase order number if associated
    int transfer_store_idx; // -1 if not a transfer, otherwise store index
    SpecialOrderType type;
    char serial_number[NAME_LEN];
} SpecialOrder;

/* Transaction: a completed or in-progress sale record with payment tracking.
 * status: 0=open layaway, 1=completed, 2=paid in full
 */
typedef struct {
    char transaction_id[NAME_LEN];
    int customer_idx; // index into store's customers array (-1 if not linked)
    int item_count;
    char item_sku[MAX_PRODUCTS][NAME_LEN];
    double item_price[MAX_PRODUCTS]; // price at time of sale
    int qty[MAX_PRODUCTS];
    int is_special_order[MAX_PRODUCTS]; // 1 if special ordered, 0=normal stock
    char so_comments[MAX_PRODUCTS][NAME_LEN]; // special order notes per item
    int special_order_id[MAX_PRODUCTS]; // ID of special order if applicable
    int so_status[MAX_PRODUCTS]; // status of special order
    double total;
    double amount_paid;
    PaymentType payment_type; // 0=cash, 1=credit, 2=debit, 3=gift
    int status; // 0=open/layaway, 1=completed, 2=paid in full
    char notes[NAME_LEN * 2];
    char date[NAME_LEN]; // YYYY-MM-DD format or empty
    int print_receipt; // 1=yes, 0=no
    int is_return; // 1 if this is a return transaction
    char original_transaction_id[NAME_LEN]; // original sale's ID for returns
} Transaction;

typedef struct {
    int enable_campaign_emails; // 1=on,0=off
    char proof_email[NAME_LEN];
    char facebook_url[NAME_LEN];
    char twitter_url[NAME_LEN];
    char survey_url[NAME_LEN];
    int post_purchase_email_enabled;
    char post_purchase_message[NAME_LEN * 2];
} TrekMarketingSettings;

typedef struct {
    char description[NAME_LEN * 2];
    int requires_tax_id;
    int hidden;
} TaxExceptionReason;

typedef struct {
    char user_name[NAME_LEN];
    char start_time[NAME_LEN]; // YYYY-MM-DD HH:MM
    int has_end_time;
    char end_time[NAME_LEN]; // YYYY-MM-DD HH:MM
    int hidden;
} TimeClockEntry;

typedef struct {
    char name[NAME_LEN];
    char location[NAME_LEN];
    double monthly_goal;
    double sales_to_date;
    int transactions;
    TrekMarketingSettings marketing;
    Product products[MAX_PRODUCTS];
    int product_count;
    Customer customers[MAX_CUSTOMERS];
    int customer_count;
    double daily_sales[MAX_DAYS];
    int day_count;
    Quote quotes[MAX_QUOTES];
    int quote_count;
    Transaction sales[MAX_TRANSACTIONS];
    int sales_count;
    SpecialOrder special_orders[MAX_SPECIAL_ORDERS];
    int special_order_count;
} Store;

static Store stores[MAX_STORES];
static int store_count = 0;
static TaxExceptionReason tax_exceptions[MAX_TAX_EXCEPTIONS];
static int tax_exception_count = 0;
static TimeClockEntry time_clock_entries[MAX_TIME_CLOCK_ENTRIES];
static int time_clock_count = 0;
static GtkWidget *main_window;
static GtkWidget *status_label;

#define MAX_TILES 20
#define TILE_SIZE_SMALL 100
#define TILE_SIZE_WIDE 212
#define TILE_SIZE_LARGE 212

typedef enum { TILE_ADD_STORE, TILE_LIST_STORES, TILE_STORE_INFO, TILE_INVENTORY, TILE_CUSTOMERS, TILE_SALES, TILE_RETURN, TILE_LAYAWAY, TILE_BUSINESS, TILE_TREND, TILE_SAVE, TILE_LOAD, TILE_EXIT } TileType;
typedef enum { SIZE_SMALL, SIZE_WIDE, SIZE_LARGE } TileSize;
typedef enum { COLOR_LIGHT_BLUE, COLOR_DARK_BLUE, COLOR_GREEN, COLOR_ORANGE, COLOR_SLATE } TileColor;

typedef struct {
    TileType type;
    char label[NAME_LEN];
    int x, y;
    int width, height;
    TileSize size;
    TileColor color;
    int visible;
} Tile;

static Tile tiles[MAX_TILES];
static int tile_count = 11;
static int desktop_locked = 1;
static int dragging_tile = -1;
static int drag_start_x, drag_start_y;
static GtkWidget *desktop_canvas = NULL;

// Forward declarations
static void show_main_menu(void);
static void add_store_dialog(void);
static void list_stores_dialog(void);
static void store_info_dialog(void);
static int choose_store_index(void);
static void add_product_dialog(Store *s);
static void manage_inventory_dialog(void);
static void business_menu_dialog(void);
static void trek_marketing_settings_dialog(void);
static void manage_customers_dialog(void);
static void add_customer_dialog(Store *s);
static void list_customers_dialog(Store *s);
static void create_sale_dialog(void);
static void create_layaway_dialog(void);
static void complete_sale_dialog(void);
static void create_return_dialog(void);
static void customer_tax_exceptions_dialog(void);
static void on_add_tax_exception_clicked(GtkToolButton *button, gpointer data);
static void time_clock_dialog(void);
static void on_add_time_clock_clicked(GtkToolButton *button, gpointer data);
static void on_edit_time_clock_clicked(GtkToolButton *button, gpointer data);
static void on_remove_time_clock_clicked(GtkToolButton *button, gpointer data);
static void on_restore_time_clock_clicked(GtkToolButton *button, gpointer data);
static void on_refresh_time_clock_clicked(GtkButton *button, gpointer data);
static void time_clock_report_dialog(void);
static void refresh_time_clock_report(GtkWidget *dialog);
static void on_refresh_time_clock_report_clicked(GtkButton *button, gpointer data);
static void special_order_prompt_dialog(Store *s, const char *sku, int qty, int *is_special_order, char *comments);
static void reorder_list_dialog(void);
static void special_orders_on_order_report(void);
static void special_orders_not_received_report(void);
static void special_orders_received_report(void);
static void view_special_orders_dialog(void);
static void view_layaways_dialog(void);
static void special_order_notification_dialog(SpecialOrder *so);
static void mark_special_orders_received(GtkWidget *widget, gpointer data);
static void send_special_order_notifications(GtkWidget *widget, gpointer data);
static void show_trend_dialog(void);
static void save_data(void);
static void load_data(void);
static void execute_tile_action(TileType type);
static void trek_marketing_settings_dialog(void);
static void show_trend_dialog(void);
static void save_data(void);
static void load_data(void);

// Utility function to create a dialog
static GtkWidget* create_dialog(const char *title, GtkWindow *parent) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(title, parent,
                                                   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   "_OK", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 300);
    return dialog;
}

// Utility function to create a labeled entry
static GtkWidget* create_labeled_entry(const char *label_text, GtkWidget *container) {
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *label = gtk_label_new(label_text);
    GtkWidget *entry = gtk_entry_new();

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(container), hbox, FALSE, FALSE, 0);

    return entry;
}

// Utility function to show info dialog
static void show_info_dialog(const char *message) {
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_INFO,
                                              GTK_BUTTONS_OK,
                                              "%s", message);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Utility function to show error dialog
static void show_error_dialog(const char *message) {
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_ERROR,
                                              GTK_BUTTONS_OK,
                                              "%s", message);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Utility to choose a store index for dialogs (returns -1 if canceled)
static int choose_store_index(void) {
    if (store_count == 0) {
        show_error_dialog("No stores available. Add a store first.");
        return -1;
    }

    GtkWidget *dialog = create_dialog("Choose Store", GTK_WINDOW(main_window));
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *combo = gtk_combo_box_text_new();

    for (int i = 0; i < store_count; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), stores[i].name);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    gtk_container_add(GTK_CONTAINER(content_area), combo);
    gtk_widget_show_all(dialog);

    int chosen = -1;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        int active = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
        if (active >= 0 && active < store_count) chosen = active;
    }
    gtk_widget_destroy(dialog);
    return chosen;
}

// Tile system functions
static void get_tile_color(TileColor color, double *r, double *g, double *b) {
    switch (color) {
        case COLOR_LIGHT_BLUE: *r = 0.68; *g = 0.85; *b = 0.90; break;
        case COLOR_DARK_BLUE: *r = 0.20; *g = 0.40; *b = 0.60; break;
        case COLOR_GREEN: *r = 0.56; *g = 0.73; *b = 0.56; break;
        case COLOR_ORANGE: *r = 0.95; *g = 0.64; *b = 0.38; break;
        case COLOR_SLATE: *r = 0.44; *g = 0.50; *b = 0.56; break;
    }
}

static gboolean on_desktop_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    cairo_set_source_rgb(cr, 0.95, 0.95, 0.95);
    cairo_paint(cr);

    for (int i = 0; i < tile_count; i++) {
        if (!tiles[i].visible) continue;
        Tile *t = &tiles[i];
        double r, g, b;
        get_tile_color(t->color, &r, &g, &b);
        cairo_set_source_rgb(cr, r, g, b);
        cairo_rectangle(cr, t->x, t->y, t->width, t->height);
        cairo_fill(cr);

        if (!desktop_locked) {
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_set_line_width(cr, 2);
            cairo_rectangle(cr, t->x, t->y, t->width, t->height);
            cairo_stroke(cr);
        }

        cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
        cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 12);
        cairo_move_to(cr, t->x + 10, t->y + t->height / 2 + 5);
        cairo_show_text(cr, t->label);
    }

    return FALSE;
}

static int find_tile_at(int x, int y) {
    for (int i = 0; i < tile_count; i++) {
        if (!tiles[i].visible) continue;
        if (x >= tiles[i].x && x < tiles[i].x + tiles[i].width &&
            y >= tiles[i].y && y < tiles[i].y + tiles[i].height) {
            return i;
        }
    }
    return -1;
}

static gboolean on_desktop_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (event->button == 1) {
        int tile_idx = find_tile_at(event->x, event->y);
        if (tile_idx >= 0) {
            if (desktop_locked) {
                // Execute tile action if desktop is locked
                execute_tile_action(tiles[tile_idx].type);
            } else {
                // Start dragging if desktop is unlocked
                dragging_tile = tile_idx;
                drag_start_x = event->x - tiles[tile_idx].x;
                drag_start_y = event->y - tiles[tile_idx].y;
            }
        }
    } else if (event->button == 3) {
        if (desktop_locked) {
            GtkWidget *menu = gtk_menu_new();
            GtkWidget *item_unlock = gtk_menu_item_new_with_label("Unlock Desktop");
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_unlock);
            g_signal_connect_swapped(item_unlock, "activate", G_CALLBACK(gtk_menu_item_activate), item_unlock);
            gtk_widget_show_all(menu);
            gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
            desktop_locked = 0;
            gtk_label_set_text(GTK_LABEL(status_label), "Ready (Desktop Unlocked - Drag tiles)");
            gtk_widget_queue_draw(widget);
        } else {
            int tile_idx = find_tile_at(event->x, event->y);
            if (tile_idx >= 0) {
                GtkWidget *menu = gtk_menu_new();
                GtkWidget *item_small = gtk_menu_item_new_with_label("Size: Small");
                GtkWidget *item_wide = gtk_menu_item_new_with_label("Size: Wide");
                GtkWidget *item_large = gtk_menu_item_new_with_label("Size: Large");
                GtkWidget *item_color = gtk_menu_item_new_with_label("Change Color");
                GtkWidget *item_hide = gtk_menu_item_new_with_label("Hide Tile");

                gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_small);
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_wide);
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_large);
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_color);
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_hide);

                gtk_widget_show_all(menu);
                gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
            } else {
                GtkWidget *menu = gtk_menu_new();
                GtkWidget *item_lock = gtk_menu_item_new_with_label("Lock Desktop");
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_lock);
                gtk_widget_show_all(menu);
                gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
                desktop_locked = 1;
                gtk_label_set_text(GTK_LABEL(status_label), "Ready (Desktop Locked)");
                gtk_widget_queue_draw(widget);
            }
        }
    }
    return FALSE;
}

static gboolean on_desktop_motion(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    if (dragging_tile >= 0 && !desktop_locked) {
        tiles[dragging_tile].x = event->x - drag_start_x;
        tiles[dragging_tile].y = event->y - drag_start_y;
        gtk_widget_queue_draw(widget);
    }
    return FALSE;
}

static gboolean on_desktop_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    dragging_tile = -1;
    return FALSE;
}

static void init_default_tiles(void) {
    tile_count = 13;
    int x = 20, y = 80;

    tiles[0] = (Tile){ TILE_ADD_STORE, "Add Store", x, y, TILE_SIZE_SMALL, TILE_SIZE_SMALL, SIZE_SMALL, COLOR_LIGHT_BLUE, 1 };
    tiles[1] = (Tile){ TILE_LIST_STORES, "List Stores", x + 120, y, TILE_SIZE_SMALL, TILE_SIZE_SMALL, SIZE_SMALL, COLOR_LIGHT_BLUE, 1 };
    tiles[2] = (Tile){ TILE_STORE_INFO, "Store Info", x + 240, y, TILE_SIZE_SMALL, TILE_SIZE_SMALL, SIZE_SMALL, COLOR_LIGHT_BLUE, 1 };
    tiles[3] = (Tile){ TILE_INVENTORY, "Inventory", x, y + 120, TILE_SIZE_SMALL, TILE_SIZE_SMALL, SIZE_SMALL, COLOR_GREEN, 1 };
    tiles[4] = (Tile){ TILE_CUSTOMERS, "Customers", x + 120, y + 120, TILE_SIZE_SMALL, TILE_SIZE_SMALL, SIZE_SMALL, COLOR_DARK_BLUE, 1 };
    tiles[5] = (Tile){ TILE_SALES, "Sales", x + 240, y + 120, TILE_SIZE_SMALL, TILE_SIZE_SMALL, SIZE_SMALL, COLOR_ORANGE, 1 };
        tiles[12] = (Tile){ TILE_RETURN, "Return", x + 360, y + 120, TILE_SIZE_SMALL, TILE_SIZE_SMALL, SIZE_SMALL, COLOR_ORANGE, 1 };
    tiles[6] = (Tile){ TILE_LAYAWAY, "Layaway", x, y + 240, TILE_SIZE_SMALL, TILE_SIZE_SMALL, SIZE_SMALL, COLOR_ORANGE, 1 };
    tiles[7] = (Tile){ TILE_BUSINESS, "Business", x + 120, y + 240, TILE_SIZE_SMALL, TILE_SIZE_SMALL, SIZE_SMALL, COLOR_ORANGE, 1 };
    tiles[8] = (Tile){ TILE_TREND, "Trend Chart", x + 240, y + 240, TILE_SIZE_SMALL, TILE_SIZE_SMALL, SIZE_SMALL, COLOR_DARK_BLUE, 1 };
    tiles[9] = (Tile){ TILE_SAVE, "Save Data", x, y + 360, TILE_SIZE_SMALL, TILE_SIZE_SMALL, SIZE_SMALL, COLOR_SLATE, 1 };
    tiles[10] = (Tile){ TILE_LOAD, "Load Data", x + 120, y + 360, TILE_SIZE_SMALL, TILE_SIZE_SMALL, SIZE_SMALL, COLOR_SLATE, 1 };
    tiles[11] = (Tile){ TILE_EXIT, "Exit", x + 240, y + 360, TILE_SIZE_SMALL, TILE_SIZE_SMALL, SIZE_SMALL, COLOR_LIGHT_BLUE, 1 };
}

static void execute_tile_action(TileType type) {
    switch (type) {
        case TILE_ADD_STORE: add_store_dialog(); break;
        case TILE_LIST_STORES: list_stores_dialog(); break;
        case TILE_STORE_INFO: store_info_dialog(); break;
        case TILE_INVENTORY: manage_inventory_dialog(); break;
        case TILE_CUSTOMERS: manage_customers_dialog(); break;
        case TILE_SALES: create_sale_dialog(); break;
        case TILE_LAYAWAY: create_layaway_dialog(); break;
            case TILE_RETURN: create_return_dialog(); break;
        case TILE_BUSINESS: business_menu_dialog(); break;
        case TILE_TREND: show_trend_dialog(); break;
        case TILE_SAVE: save_data(); break;
        case TILE_LOAD: load_data(); break;
        case TILE_EXIT: gtk_main_quit(); break;
    }
}

// Main menu button callbacks
static void on_add_store_clicked(GtkButton *button, gpointer user_data) {
    add_store_dialog();
}

static void on_list_stores_clicked(GtkButton *button, gpointer user_data) {
    list_stores_dialog();
}

static void on_store_info_clicked(GtkButton *button, gpointer user_data) {
    store_info_dialog();
}

static void on_inventory_clicked(GtkButton *button, gpointer user_data) {
    manage_inventory_dialog();
}

static void on_business_clicked(GtkButton *button, gpointer user_data) {
    business_menu_dialog();
}

static void on_trend_clicked(GtkButton *button, gpointer user_data) {
    show_trend_dialog();
}

static void on_save_clicked(GtkButton *button, gpointer user_data) {
    save_data();
    show_info_dialog("Data saved successfully!");
}

static void on_load_clicked(GtkButton *button, gpointer user_data) {
    load_data();
    show_info_dialog("Data loaded successfully!");
}

static void on_exit_clicked(GtkButton *button, gpointer user_data) {
    gtk_main_quit();
}

static gboolean on_main_window_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    if (event->keyval == GDK_KEY_F10) {
        complete_sale_dialog();
        return TRUE; // Event handled
    }
    return FALSE; // Event not handled
}

// Create the main menu
static void show_main_menu(void) {
    init_default_tiles();

    // Clear main window
    GList *children = gtk_container_get_children(GTK_CONTAINER(main_window));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(main_window), vbox);

    // Menu bar
    GtkWidget *menubar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    // View menu
    GtkWidget *view_menu = gtk_menu_new();
    GtkWidget *view_item = gtk_menu_item_new_with_label("View");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(view_item), view_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), view_item);

    GtkWidget *layaways_item = gtk_menu_item_new_with_label("Layaways");
    g_signal_connect(layaways_item, "activate", G_CALLBACK(view_layaways_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), layaways_item);

    GtkWidget *tax_exceptions_item = gtk_menu_item_new_with_label("Customer Tax Exceptions");
    g_signal_connect(tax_exceptions_item, "activate", G_CALLBACK(customer_tax_exceptions_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), tax_exceptions_item);

    GtkWidget *time_clock_item = gtk_menu_item_new_with_label("Time Clock");
    g_signal_connect(time_clock_item, "activate", G_CALLBACK(time_clock_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), time_clock_item);

    // Sales menu
    GtkWidget *sales_menu = gtk_menu_new();
    GtkWidget *sales_item = gtk_menu_item_new_with_label("Sales");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(sales_item), sales_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), sales_item);

    GtkWidget *return_item = gtk_menu_item_new_with_label("Create a Return");
    g_signal_connect(return_item, "activate", G_CALLBACK(create_return_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(sales_menu), return_item);

    GtkWidget *special_orders_item = gtk_menu_item_new_with_label("Special Ordered Items");
    g_signal_connect(special_orders_item, "activate", G_CALLBACK(view_special_orders_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), special_orders_item);

    // Reports menu
    GtkWidget *reports_menu = gtk_menu_new();
    GtkWidget *reports_item = gtk_menu_item_new_with_label("Reports");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(reports_item), reports_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), reports_item);

    GtkWidget *reorder_list_item = gtk_menu_item_new_with_label("Reorder List");
    g_signal_connect(reorder_list_item, "activate", G_CALLBACK(reorder_list_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(reports_menu), reorder_list_item);

    GtkWidget *so_on_order_item = gtk_menu_item_new_with_label("Special Order Items On Order");
    g_signal_connect(so_on_order_item, "activate", G_CALLBACK(special_orders_on_order_report), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(reports_menu), so_on_order_item);

    GtkWidget *so_not_received_item = gtk_menu_item_new_with_label("Special Order Items Not Yet Received");
    g_signal_connect(so_not_received_item, "activate", G_CALLBACK(special_orders_not_received_report), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(reports_menu), so_not_received_item);

    GtkWidget *so_received_item = gtk_menu_item_new_with_label("Special Order Items Received");
    g_signal_connect(so_received_item, "activate", G_CALLBACK(special_orders_received_report), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(reports_menu), so_received_item);

    GtkWidget *time_clock_report_item = gtk_menu_item_new_with_label("Time Clock");
    g_signal_connect(time_clock_report_item, "activate", G_CALLBACK(time_clock_report_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(reports_menu), time_clock_report_item);

    // Title bar
    GtkWidget *title_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), title_box, FALSE, FALSE, 0);
    gtk_widget_set_size_request(title_box, -1, 50);

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='large' weight='bold'>Ascend Retail Platform</span>");
    gtk_box_pack_start(GTK_BOX(title_box), title, FALSE, FALSE, 15);

    // Desktop canvas
    desktop_canvas = gtk_drawing_area_new();
    gtk_widget_set_size_request(desktop_canvas, 800, 500);
    gtk_box_pack_start(GTK_BOX(vbox), desktop_canvas, TRUE, TRUE, 0);

    g_signal_connect(desktop_canvas, "draw", G_CALLBACK(on_desktop_draw), NULL);
    g_signal_connect(desktop_canvas, "button-press-event", G_CALLBACK(on_desktop_button_press), NULL);
    g_signal_connect(desktop_canvas, "motion-notify-event", G_CALLBACK(on_desktop_motion), NULL);
    g_signal_connect(desktop_canvas, "button-release-event", G_CALLBACK(on_desktop_button_release), NULL);

    gtk_widget_set_events(desktop_canvas, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);

    // Status bar
    GtkWidget *status_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_end(GTK_BOX(vbox), status_box, FALSE, FALSE, 0);
    gtk_widget_set_size_request(status_box, -1, 30);

    status_label = gtk_label_new("Ready (Desktop Locked)");
    gtk_box_pack_start(GTK_BOX(status_box), status_label, FALSE, FALSE, 10);

    gtk_widget_show_all(main_window);
}

static void on_add_tax_exception_clicked(GtkToolButton *button, gpointer data) {
    (void)button;

    GtkWidget *parent_dialog = GTK_WIDGET(data);
    GtkListStore *store_list = GTK_LIST_STORE(g_object_get_data(G_OBJECT(parent_dialog), "tax_exception_store"));
    if (!store_list) return;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Add Tax Exception Reason",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Save", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *description_entry = create_labeled_entry("Description:", vbox);

    GtkWidget *requires_tax_id = gtk_check_button_new_with_label("Requires Tax ID");
    gtk_box_pack_start(GTK_BOX(vbox), requires_tax_id, FALSE, FALSE, 0);

    GtkWidget *hidden = gtk_check_button_new_with_label("Hidden");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hidden), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), hidden, FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *desc = gtk_entry_get_text(GTK_ENTRY(description_entry));
        int req_tax_id = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(requires_tax_id));
        int is_hidden = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hidden));

        if (strlen(desc) == 0) {
            show_error_dialog("Description is required.");
        } else if (tax_exception_count >= MAX_TAX_EXCEPTIONS) {
            show_error_dialog("Maximum tax exception reasons reached.");
        } else {
            TaxExceptionReason *r = &tax_exceptions[tax_exception_count++];
            strncpy(r->description, desc, sizeof(r->description) - 1);
            r->description[sizeof(r->description) - 1] = '\0';
            r->requires_tax_id = req_tax_id;
            r->hidden = is_hidden;

            GtkTreeIter iter;
            gtk_list_store_append(store_list, &iter);
            gtk_list_store_set(store_list, &iter,
                              0, r->description,
                              1, r->requires_tax_id ? "Yes" : "No",
                              2, r->hidden ? "Yes" : "No",
                              -1);

            save_data();
            show_info_dialog("Tax exception reason saved.");
        }
    }

    gtk_widget_destroy(dialog);
}

static void customer_tax_exceptions_dialog(void) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Customer Tax Exceptions",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Close", GTK_RESPONSE_CLOSE,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);
    GtkToolItem *add_item = gtk_tool_button_new(NULL, "Add");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(add_item), "list-add");
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), add_item, -1);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    GtkWidget *list_sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(list_sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(list_sw, 560, 260);
    gtk_box_pack_start(GTK_BOX(vbox), list_sw, TRUE, TRUE, 0);

    GtkListStore *store_list = gtk_list_store_new(3,
                                                   G_TYPE_STRING,
                                                   G_TYPE_STRING,
                                                   G_TYPE_STRING);

    for (int i = 0; i < tax_exception_count; i++) {
        TaxExceptionReason *r = &tax_exceptions[i];
        GtkTreeIter iter;
        gtk_list_store_append(store_list, &iter);
        gtk_list_store_set(store_list, &iter,
                          0, r->description,
                          1, r->requires_tax_id ? "Yes" : "No",
                          2, r->hidden ? "Yes" : "No",
                          -1);
    }

    GtkTreeView *tree = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_list)));
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Description", renderer, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Requires Tax ID", renderer, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Hidden", renderer, "text", 2, NULL);
    gtk_container_add(GTK_CONTAINER(list_sw), GTK_WIDGET(tree));

    g_object_set_data(G_OBJECT(dialog), "tax_exception_store", store_list);
    g_signal_connect(add_item, "clicked", G_CALLBACK(on_add_tax_exception_clicked), dialog);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void get_today_date(char *out, size_t out_size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(out, out_size, "%Y-%m-%d", tm_info);
}

static int parse_datetime_str(const char *value, struct tm *out_tm) {
    int year, month, day, hour, minute;
    if (sscanf(value, "%d-%d-%d %d:%d", &year, &month, &day, &hour, &minute) != 5) {
        return 0;
    }
    memset(out_tm, 0, sizeof(struct tm));
    out_tm->tm_year = year - 1900;
    out_tm->tm_mon = month - 1;
    out_tm->tm_mday = day;
    out_tm->tm_hour = hour;
    out_tm->tm_min = minute;
    out_tm->tm_isdst = -1;
    return 1;
}

static double compute_time_clock_hours(const TimeClockEntry *entry) {
    if (!entry->has_end_time) return 0.0;

    struct tm start_tm, end_tm;
    if (!parse_datetime_str(entry->start_time, &start_tm) || !parse_datetime_str(entry->end_time, &end_tm)) {
        return 0.0;
    }

    time_t start_ts = mktime(&start_tm);
    time_t end_ts = mktime(&end_tm);
    if (start_ts == (time_t)-1 || end_ts == (time_t)-1 || end_ts < start_ts) {
        return 0.0;
    }

    double seconds = difftime(end_ts, start_ts);
    return seconds / 3600.0;
}

static int selected_time_clock_index(GtkWidget *dialog) {
    GtkWidget *tree = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "time_clock_tree"));
    if (!tree) return -1;

    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    GtkTreeModel *model;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) return -1;

    int idx = -1;
    gtk_tree_model_get(model, &iter, 0, &idx, -1);
    return idx;
}

static void refresh_time_clock_store(GtkWidget *dialog) {
    GtkListStore *store_list = GTK_LIST_STORE(g_object_get_data(G_OBJECT(dialog), "time_clock_store"));
    GtkWidget *from_entry = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "time_clock_from"));
    GtkWidget *to_entry = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "time_clock_to"));
    GtkWidget *show_hidden = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "time_clock_show_hidden"));
    if (!store_list || !from_entry || !to_entry || !show_hidden) return;

    const char *from_date = gtk_entry_get_text(GTK_ENTRY(from_entry));
    const char *to_date = gtk_entry_get_text(GTK_ENTRY(to_entry));
    int include_hidden = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(show_hidden));

    gtk_list_store_clear(store_list);

    for (int i = 0; i < time_clock_count; i++) {
        TimeClockEntry *e = &time_clock_entries[i];
        if (!include_hidden && e->hidden) continue;

        char entry_date[11] = "";
        strncpy(entry_date, e->start_time, 10);
        entry_date[10] = '\0';
        if (strlen(from_date) >= 10 && strcmp(entry_date, from_date) < 0) continue;
        if (strlen(to_date) >= 10 && strcmp(entry_date, to_date) > 0) continue;

        char hours_str[32];
        if (e->has_end_time) {
            sprintf(hours_str, "%.2f", compute_time_clock_hours(e));
        } else {
            strcpy(hours_str, "Open");
        }

        GtkTreeIter iter;
        gtk_list_store_append(store_list, &iter);
        gtk_list_store_set(store_list, &iter,
                          0, i,
                          1, e->user_name,
                          2, e->start_time,
                          3, e->has_end_time ? e->end_time : "(Still Working)",
                          4, hours_str,
                          5, e->hidden ? "Yes" : "No",
                          -1);
    }
}

static void on_refresh_time_clock_clicked(GtkButton *button, gpointer data) {
    (void)button;
    refresh_time_clock_store(GTK_WIDGET(data));
}

static void open_time_clock_entry_editor(GtkWidget *parent_dialog, int edit_index) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(edit_index >= 0 ? "Edit Time Clock Entry" : "Add Time Clock Entry",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Save", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *user_combo = gtk_combo_box_text_new_with_entry();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(user_combo), "Manager");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(user_combo), "Sales Associate");
    for (int i = 0; i < time_clock_count; i++) {
        if (strlen(time_clock_entries[i].user_name) > 0) {
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(user_combo), time_clock_entries[i].user_name);
        }
    }
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("User:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), user_combo, FALSE, FALSE, 0);

    GtkWidget *start_entry = create_labeled_entry("Start Time (YYYY-MM-DD HH:MM):", vbox);
    GtkWidget *has_end_check = gtk_check_button_new_with_label("End Time");
    gtk_box_pack_start(GTK_BOX(vbox), has_end_check, FALSE, FALSE, 0);
    GtkWidget *end_entry = create_labeled_entry("End Time (YYYY-MM-DD HH:MM):", vbox);

    if (edit_index >= 0 && edit_index < time_clock_count) {
        TimeClockEntry *e = &time_clock_entries[edit_index];
        GtkWidget *user_entry = gtk_bin_get_child(GTK_BIN(user_combo));
        gtk_entry_set_text(GTK_ENTRY(user_entry), e->user_name);
        gtk_entry_set_text(GTK_ENTRY(start_entry), e->start_time);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(has_end_check), e->has_end_time);
        gtk_entry_set_text(GTK_ENTRY(end_entry), e->end_time);
    } else {
        char now_dt[NAME_LEN];
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        strftime(now_dt, sizeof(now_dt), "%Y-%m-%d %H:%M", tm_info);
        gtk_entry_set_text(GTK_ENTRY(start_entry), now_dt);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(has_end_check), 1);
        gtk_entry_set_text(GTK_ENTRY(end_entry), now_dt);
    }

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        GtkWidget *user_entry = gtk_bin_get_child(GTK_BIN(user_combo));
        const char *user_name = gtk_entry_get_text(GTK_ENTRY(user_entry));
        const char *start_time = gtk_entry_get_text(GTK_ENTRY(start_entry));
        int has_end = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(has_end_check));
        const char *end_time = gtk_entry_get_text(GTK_ENTRY(end_entry));

        struct tm test_tm;
        if (strlen(user_name) == 0) {
            show_error_dialog("User is required.");
        } else if (!parse_datetime_str(start_time, &test_tm)) {
            show_error_dialog("Start Time must use format YYYY-MM-DD HH:MM.");
        } else if (has_end && !parse_datetime_str(end_time, &test_tm)) {
            show_error_dialog("End Time must use format YYYY-MM-DD HH:MM.");
        } else if (edit_index < 0 && time_clock_count >= MAX_TIME_CLOCK_ENTRIES) {
            show_error_dialog("Maximum time clock entries reached.");
        } else {
            TimeClockEntry *entry;
            if (edit_index >= 0 && edit_index < time_clock_count) {
                entry = &time_clock_entries[edit_index];
            } else {
                entry = &time_clock_entries[time_clock_count++];
                entry->hidden = 0;
            }
            strncpy(entry->user_name, user_name, NAME_LEN - 1);
            entry->user_name[NAME_LEN - 1] = '\0';
            strncpy(entry->start_time, start_time, NAME_LEN - 1);
            entry->start_time[NAME_LEN - 1] = '\0';
            entry->has_end_time = has_end;
            if (has_end) {
                strncpy(entry->end_time, end_time, NAME_LEN - 1);
                entry->end_time[NAME_LEN - 1] = '\0';
            } else {
                strcpy(entry->end_time, "");
            }

            save_data();
            refresh_time_clock_store(parent_dialog);
            show_info_dialog("Time clock entry saved.");
        }
    }

    gtk_widget_destroy(dialog);
}

static void on_add_time_clock_clicked(GtkToolButton *button, gpointer data) {
    (void)button;
    open_time_clock_entry_editor(GTK_WIDGET(data), -1);
}

static void on_edit_time_clock_clicked(GtkToolButton *button, gpointer data) {
    (void)button;
    GtkWidget *dialog = GTK_WIDGET(data);
    int idx = selected_time_clock_index(dialog);
    if (idx < 0 || idx >= time_clock_count) {
        show_error_dialog("Select a time clock entry to modify.");
        return;
    }
    open_time_clock_entry_editor(dialog, idx);
}

static void on_remove_time_clock_clicked(GtkToolButton *button, gpointer data) {
    (void)button;
    GtkWidget *dialog = GTK_WIDGET(data);
    int idx = selected_time_clock_index(dialog);
    if (idx < 0 || idx >= time_clock_count) {
        show_error_dialog("Select a time clock entry to remove.");
        return;
    }
    time_clock_entries[idx].hidden = 1;
    save_data();
    refresh_time_clock_store(dialog);
    show_info_dialog("Time clock entry removed.");
}

static void on_restore_time_clock_clicked(GtkToolButton *button, gpointer data) {
    (void)button;
    GtkWidget *dialog = GTK_WIDGET(data);
    int idx = selected_time_clock_index(dialog);
    if (idx < 0 || idx >= time_clock_count) {
        show_error_dialog("Select a hidden time clock entry to restore.");
        return;
    }
    time_clock_entries[idx].hidden = 0;
    save_data();
    refresh_time_clock_store(dialog);
    show_info_dialog("Time clock entry restored.");
}

static void time_clock_dialog(void) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Time Clock",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Close", GTK_RESPONSE_CLOSE,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);
    GtkToolItem *add_item = gtk_tool_button_new(NULL, "Add");
    GtkToolItem *edit_item = gtk_tool_button_new(NULL, "Modify");
    GtkToolItem *remove_item = gtk_tool_button_new(NULL, "Remove");
    GtkToolItem *restore_item = gtk_tool_button_new(NULL, "Restore");
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), add_item, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), edit_item, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), remove_item, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), restore_item, -1);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    GtkWidget *filter_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(vbox), filter_box, FALSE, FALSE, 0);
    GtkWidget *from_entry = gtk_entry_new();
    GtkWidget *to_entry = gtk_entry_new();
    char today[NAME_LEN];
    get_today_date(today, sizeof(today));
    gtk_entry_set_text(GTK_ENTRY(from_entry), today);
    gtk_entry_set_text(GTK_ENTRY(to_entry), today);
    gtk_box_pack_start(GTK_BOX(filter_box), gtk_label_new("From (YYYY-MM-DD):"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(filter_box), from_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(filter_box), gtk_label_new("To (YYYY-MM-DD):"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(filter_box), to_entry, FALSE, FALSE, 0);
    GtkWidget *show_hidden = gtk_check_button_new_with_label("Show Hidden");
    gtk_box_pack_start(GTK_BOX(filter_box), show_hidden, FALSE, FALSE, 0);
    GtkWidget *refresh_btn = gtk_button_new_with_label("Refresh");
    gtk_box_pack_start(GTK_BOX(filter_box), refresh_btn, FALSE, FALSE, 0);

    GtkWidget *list_sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(list_sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(list_sw, 760, 320);
    gtk_box_pack_start(GTK_BOX(vbox), list_sw, TRUE, TRUE, 0);

    GtkListStore *store_list = gtk_list_store_new(6,
                                                   G_TYPE_INT,
                                                   G_TYPE_STRING,
                                                   G_TYPE_STRING,
                                                   G_TYPE_STRING,
                                                   G_TYPE_STRING,
                                                   G_TYPE_STRING);

    GtkTreeView *tree = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_list)));
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(tree, -1, "User", renderer, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Start Time", renderer, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "End Time", renderer, "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Hours", renderer, "text", 4, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Hidden", renderer, "text", 5, NULL);
    gtk_container_add(GTK_CONTAINER(list_sw), GTK_WIDGET(tree));

    g_object_set_data(G_OBJECT(dialog), "time_clock_store", store_list);
    g_object_set_data(G_OBJECT(dialog), "time_clock_tree", tree);
    g_object_set_data(G_OBJECT(dialog), "time_clock_from", from_entry);
    g_object_set_data(G_OBJECT(dialog), "time_clock_to", to_entry);
    g_object_set_data(G_OBJECT(dialog), "time_clock_show_hidden", show_hidden);

    g_signal_connect(add_item, "clicked", G_CALLBACK(on_add_time_clock_clicked), dialog);
    g_signal_connect(edit_item, "clicked", G_CALLBACK(on_edit_time_clock_clicked), dialog);
    g_signal_connect(remove_item, "clicked", G_CALLBACK(on_remove_time_clock_clicked), dialog);
    g_signal_connect(restore_item, "clicked", G_CALLBACK(on_restore_time_clock_clicked), dialog);
    g_signal_connect(refresh_btn, "clicked", G_CALLBACK(on_refresh_time_clock_clicked), dialog);

    refresh_time_clock_store(dialog);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void time_clock_report_dialog(void) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Time Clock Report",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Close", GTK_RESPONSE_CLOSE,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *filter_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(vbox), filter_box, FALSE, FALSE, 0);
    GtkWidget *from_entry = gtk_entry_new();
    GtkWidget *to_entry = gtk_entry_new();
    char today[NAME_LEN];
    get_today_date(today, sizeof(today));
    gtk_entry_set_text(GTK_ENTRY(from_entry), today);
    gtk_entry_set_text(GTK_ENTRY(to_entry), today);
    gtk_box_pack_start(GTK_BOX(filter_box), gtk_label_new("From (YYYY-MM-DD):"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(filter_box), from_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(filter_box), gtk_label_new("To (YYYY-MM-DD):"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(filter_box), to_entry, FALSE, FALSE, 0);

    GtkWidget *refresh_btn = gtk_button_new_with_label("Refresh");
    gtk_box_pack_start(GTK_BOX(filter_box), refresh_btn, FALSE, FALSE, 0);

    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(sw, 700, 340);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);
    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_container_add(GTK_CONTAINER(sw), text_view);

    g_object_set_data(G_OBJECT(dialog), "time_clock_report_from", from_entry);
    g_object_set_data(G_OBJECT(dialog), "time_clock_report_to", to_entry);
    g_object_set_data(G_OBJECT(dialog), "time_clock_report_text", text_view);

    g_signal_connect(refresh_btn, "clicked", G_CALLBACK(on_refresh_time_clock_report_clicked), dialog);

    refresh_time_clock_report(dialog);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void on_refresh_time_clock_report_clicked(GtkButton *button, gpointer data) {
    (void)button;
    refresh_time_clock_report(GTK_WIDGET(data));
}

static void refresh_time_clock_report(GtkWidget *dialog) {
    GtkWidget *from_entry = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "time_clock_report_from"));
    GtkWidget *to_entry = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "time_clock_report_to"));
    GtkWidget *text_view = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "time_clock_report_text"));
    if (!from_entry || !to_entry || !text_view) return;

    const char *from_date = gtk_entry_get_text(GTK_ENTRY(from_entry));
    const char *to_date = gtk_entry_get_text(GTK_ENTRY(to_entry));
    double total_hours = 0.0;
    char report[12000] = "TIME CLOCK REPORT\n\n";
    char line[256];

    for (int i = 0; i < time_clock_count; i++) {
        TimeClockEntry *e = &time_clock_entries[i];
        if (e->hidden) continue;
        char entry_date[11] = "";
        strncpy(entry_date, e->start_time, 10);
        entry_date[10] = '\0';
        if (strlen(from_date) >= 10 && strcmp(entry_date, from_date) < 0) continue;
        if (strlen(to_date) >= 10 && strcmp(entry_date, to_date) > 0) continue;

        double h = compute_time_clock_hours(e);
        if (e->has_end_time) {
            sprintf(line, "%s | %s -> %s | %.2f hrs\n", e->user_name, e->start_time, e->end_time, h);
            total_hours += h;
        } else {
            sprintf(line, "%s | %s -> (Still Working)\n", e->user_name, e->start_time);
        }
        strncat(report, line, sizeof(report) - strlen(report) - 1);
    }

    sprintf(line, "\nTotal Closed-Shift Hours: %.2f\n", total_hours);
    strncat(report, line, sizeof(report) - strlen(report) - 1);

    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buf, report, -1);
}


// Add store dialog
static void add_store_dialog(void) {
    GtkWidget *dialog = create_dialog("Add Store", GTK_WINDOW(main_window));
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *entry_name = create_labeled_entry("Store Name:", vbox);
    GtkWidget *entry_location = create_labeled_entry("Location:", vbox);
    GtkWidget *entry_goal = create_labeled_entry("Monthly Goal ($):", vbox);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        if (store_count >= MAX_STORES) {
            show_error_dialog("Maximum stores reached!");
        } else {
            Store *s = &stores[store_count];
            const char *name = gtk_entry_get_text(GTK_ENTRY(entry_name));
            const char *location = gtk_entry_get_text(GTK_ENTRY(entry_location));
            const char *goal_str = gtk_entry_get_text(GTK_ENTRY(entry_goal));

            if (strlen(name) == 0 || strlen(location) == 0) {
                show_error_dialog("Name and location are required!");
            } else {
                strncpy(s->name, name, NAME_LEN - 1);
                strncpy(s->location, location, NAME_LEN - 1);
                s->monthly_goal = atof(goal_str);
                s->sales_to_date = 0;
                s->transactions = 0;
                s->product_count = 0;
                s->customer_count = 0;
                s->sales_count = 0;
                s->day_count = 0;
                s->quote_count = 0;
                memset(s->daily_sales, 0, sizeof(s->daily_sales));
                s->marketing.enable_campaign_emails = 0;
                s->marketing.post_purchase_email_enabled = 1;
                strcpy(s->marketing.proof_email, "");
                strcpy(s->marketing.facebook_url, "");
                strcpy(s->marketing.twitter_url, "");
                strcpy(s->marketing.survey_url, "");
                strcpy(s->marketing.post_purchase_message, "Thank you for your purchase!");
                store_count++;

                char msg[100];
                sprintf(msg, "Store '%s' added successfully!", s->name);
                show_info_dialog(msg);
            }
        }
    }

    gtk_widget_destroy(dialog);
}

// List stores dialog
static void list_stores_dialog(void) {
    GtkWidget *dialog = create_dialog("Store List", GTK_WINDOW(main_window));
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(content_area), scrolled_window);

    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    if (store_count == 0) {
        gtk_text_buffer_set_text(buffer, "No stores available.", -1);
    } else {
        char text[2000] = "";
        for (int i = 0; i < store_count; i++) {
            char line[200];
            sprintf(line, "%d. %s (%s)\n   Goal: $%.2f, Sales: $%.2f, Products: %d\n\n",
                    i + 1, stores[i].name, stores[i].location,
                    stores[i].monthly_goal, stores[i].sales_to_date, stores[i].product_count);
            strcat(text, line);
        }
        gtk_text_buffer_set_text(buffer, text, -1);
    }

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Store info dialog (simplified - would need store selection)
static void store_info_dialog(void) {
    if (store_count == 0) {
        show_error_dialog("No stores available!");
        return;
    }

    // For simplicity, show info for first store
    Store *s = &stores[0];

    char info[600];
    sprintf(info, "Store: %s\nLocation: %s\nGoal: $%.2f\nSales: $%.2f\nTransactions: %d\nProducts: %d\n",
            s->name, s->location, s->monthly_goal, s->sales_to_date,
            s->transactions, s->product_count);
    strcat(info, "\nProducts: \n");
    for (int i = 0; i < s->product_count; i++) {
        Product *p = &s->products[i];
        char line[200];
        sprintf(line, "- %s (%s) %s %s $%.2f stock=%d\n", p->name, p->sku,
                p->vendor[0] ? p->vendor : "Unknown",
                p->serialized ? "[Serialized]" : "[Non-Serialized]",
                p->price, p->stock);
        strcat(info, line);
    }

    show_info_dialog(info);
}

static void add_product_dialog(Store *s) {
    if (!s) return;
    if (s->product_count >= MAX_PRODUCTS) {
        show_error_dialog("Max products reached for store.");
        return;
    }

    GtkWidget *dialog = create_dialog("Add Product", GTK_WINDOW(main_window));
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *entry_sku = create_labeled_entry("SKU:", vbox);
    GtkWidget *entry_name = create_labeled_entry("Name:", vbox);
    GtkWidget *entry_vendor = create_labeled_entry("Vendor:", vbox);
    GtkWidget *entry_price = create_labeled_entry("Price ($):", vbox);
    GtkWidget *entry_stock = create_labeled_entry("Stock:", vbox);
    GtkWidget *serialized_check = gtk_check_button_new_with_label("Serialized (Trek-bike style)");
    gtk_box_pack_start(GTK_BOX(vbox), serialized_check, FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *sku = gtk_entry_get_text(GTK_ENTRY(entry_sku));
        const char *name = gtk_entry_get_text(GTK_ENTRY(entry_name));
        const char *vendor = gtk_entry_get_text(GTK_ENTRY(entry_vendor));
        const char *price_str = gtk_entry_get_text(GTK_ENTRY(entry_price));
        const char *stock_str = gtk_entry_get_text(GTK_ENTRY(entry_stock));

        if (strlen(sku) == 0 || strlen(name) == 0) {
            show_error_dialog("SKU and Name are required.");
        } else {
            Product *p = &s->products[s->product_count];
            strncpy(p->sku, sku, NAME_LEN - 1);
            strncpy(p->name, name, NAME_LEN - 1);
            strncpy(p->vendor, vendor[0] ? vendor : "Unknown", NAME_LEN - 1);
            p->serialized = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(serialized_check));
            p->price = atof(price_str);
            p->stock = atoi(stock_str);
            p->sold = 0;
            s->product_count++;

            show_info_dialog("Product added to store successfully.");
        }
    }

    gtk_widget_destroy(dialog);
}

static void manage_inventory_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    add_product_dialog(&stores[si]);
}

static void trek_registration_prompt(Store *s, Quote *q, int item_index) {
    if (!s || !q || item_index < 0 || item_index >= q->item_count) return;

    const char *sku = q->item_sku[item_index];
    Product *p = NULL;
    for (int i = 0; i < s->product_count; i++) {
        if (strcmp(s->products[i].sku, sku) == 0) { p = &s->products[i]; break; }
    }
    if (!p) return;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Trek Registration",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Register", 1,
                                                   "_Cancel", 2,
                                                   "_Decline", 3,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    char top[256];
    sprintf(top, "Trek bike registration for %s (SKU %s)\nCustomer: %s\nQuote: %s\nQty: %d", p->name, p->sku, q->customer, q->quote_id, q->qty[item_index]);
    GtkWidget *label = gtk_label_new(top);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    GtkWidget *gift_checkbox = gtk_check_button_new_with_label("This item is a gift box (postpone communication)");
    gtk_box_pack_start(GTK_BOX(vbox), gift_checkbox, FALSE, FALSE, 0);

    GtkWidget *date_entry = create_labeled_entry("Postpone until (YYYY-MM-DD):", vbox);
    gtk_widget_show_all(dialog);

    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    int status = 0;
    if (response == 1) {
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gift_checkbox))) {
            status = 2; // deferred
            const char *postpone = gtk_entry_get_text(GTK_ENTRY(date_entry));
            strncpy(q->postpone_date[item_index], postpone, NAME_LEN - 1);
            if (strlen(postpone) == 0) strcpy(q->postpone_date[item_index], "(no date)");
        } else {
            status = 1; // registered now
            strcpy(q->postpone_date[item_index], "");
        }
    } else if (response == 2) {
        status = 3; // skipped, ask next time
    } else if (response == 3) {
        status = 4; // declined forever
    }
    q->register_status[item_index] = status;

    gtk_widget_destroy(dialog);
}

static void create_quote_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];

    if (s->quote_count >= MAX_QUOTES) {
        show_error_dialog("Max quotes reached in this store.");
        return;
    }

    GtkWidget *dialog = create_dialog("Create Quote", GTK_WINDOW(main_window));
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *entry_customer = create_labeled_entry("Customer:", vbox);
    GtkWidget *entry_quote_id = create_labeled_entry("Quote ID:", vbox);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK) {
        gtk_widget_destroy(dialog);
        return;
    }

    Quote q;
    memset(&q, 0, sizeof(Quote));
    strncpy(q.customer, gtk_entry_get_text(GTK_ENTRY(entry_customer)), NAME_LEN - 1);
    strncpy(q.quote_id, gtk_entry_get_text(GTK_ENTRY(entry_quote_id)), NAME_LEN - 1);
    q.total = 0;
    q.active = 1;

    gtk_widget_destroy(dialog);

    while (q.item_count < MAX_PRODUCTS) {
        GtkWidget *item_dialog = create_dialog("Add Quote Item", GTK_WINDOW(main_window));
        GtkWidget *item_area = gtk_dialog_get_content_area(GTK_DIALOG(item_dialog));
        GtkWidget *item_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_add(GTK_CONTAINER(item_area), item_box);

        GtkWidget *entry_sku = create_labeled_entry("SKU (blank when done):", item_box);
        GtkWidget *entry_qty = create_labeled_entry("Qty:", item_box);
        gtk_widget_show_all(item_dialog);

        if (gtk_dialog_run(GTK_DIALOG(item_dialog)) != GTK_RESPONSE_OK) {
            gtk_widget_destroy(item_dialog);
            break;
        }

        const char *sku = gtk_entry_get_text(GTK_ENTRY(entry_sku));
        const char *qty_str = gtk_entry_get_text(GTK_ENTRY(entry_qty));
        gtk_widget_destroy(item_dialog);

        if (strlen(sku) == 0) break;
        int qty = atoi(qty_str);
        if (qty <= 0) {
            show_error_dialog("Quantity must be positive.");
            continue;
        }

        int found = -1;
        for (int i = 0; i < s->product_count; i++) {
            if (strcmp(s->products[i].sku, sku) == 0) { found = i; break; }
        }
        if (found < 0) {
            show_error_dialog("Product SKU not found in store.");
            continue;
        }
        if (qty > s->products[found].stock) {
            show_error_dialog("Not enough stock for this product.");
            continue;
        }

        strncpy(q.item_sku[q.item_count], sku, NAME_LEN - 1);
        q.qty[q.item_count] = qty;
        q.total += s->products[found].price * qty;
        q.register_status[q.item_count] = 0;
        q.item_count++;

        char msg[128];
        sprintf(msg, "%d x %s added to quote.", qty, s->products[found].name);
        show_info_dialog(msg);
    }

    if (q.item_count == 0) {
        show_error_dialog("Quote contains no items, not saved.");
        return;
    }

    s->quotes[s->quote_count++] = q;
    char msg2[128];
    sprintf(msg2, "Quote %s created successfully (total $%.2f).", q.quote_id, q.total);
    show_info_dialog(msg2);
}

static void process_quote_to_sale_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];

    int active_count = 0;
    for (int i = 0; i < s->quote_count; i++) if (s->quotes[i].active) active_count++;
    if (active_count == 0) {
        show_error_dialog("No active quotes in this store.");
        return;
    }

    GtkWidget *dialog = create_dialog("Select Quote to Process", GTK_WINDOW(main_window));
    GtkWidget *cont = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *combo = gtk_combo_box_text_new();
    for (int i = 0; i < s->quote_count; i++) {
        if (!s->quotes[i].active) continue;
        char label[128];
        sprintf(label, "%s (%.2f)", s->quotes[i].quote_id, s->quotes[i].total);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), label);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    gtk_container_add(GTK_CONTAINER(cont), combo);
    gtk_widget_show_all(dialog);

    int quote_index = -1;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        int selected = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
        if (selected < 0) {
            gtk_widget_destroy(dialog);
            return;
        }
        // map selection to quote index
        int cnt = 0;
        for (int i = 0; i < s->quote_count; i++) {
            if (!s->quotes[i].active) continue;
            if (cnt == selected) { quote_index = i; break; }
            cnt++;
        }
    }
    gtk_widget_destroy(dialog);

    if (quote_index < 0) return;

    Quote *q = &s->quotes[quote_index];
    for (int i = 0; i < q->item_count; i++) {
        // for each Trek serialized product in the quote, require registration prompt
        Product *p = NULL;
        for (int j = 0; j < s->product_count; j++) {
            if (strcmp(s->products[j].sku, q->item_sku[i]) == 0) { p = &s->products[j]; break; }
        }
        if (!p) continue;
        if (p->serialized && strcasecmp(p->vendor, "Trek") == 0) {
            trek_registration_prompt(s, q, i);
        } else {
            q->register_status[i] = 0; // not applicable
        }

        // reduce stock and accumulate sold
        if (p->stock >= q->qty[i]) {
            p->stock -= q->qty[i];
            p->sold += q->qty[i];
        }
    }

    q->active = 0;
    s->sales_to_date += q->total;
    s->transactions += 1;
    if (s->day_count < MAX_DAYS) {
        s->daily_sales[s->day_count++] = s->sales_to_date;
    }

    show_info_dialog("Sale processed and registrations updated.");
}

static void business_menu_dialog(void) {
    GtkWidget *dialog = create_dialog("Business Menu", GTK_WINDOW(main_window));
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *btn_create_quote = gtk_button_new_with_label("Create Quote");
    GtkWidget *btn_process_sale = gtk_button_new_with_label("Process Quote to Sale");
    GtkWidget *btn_trek_marketing = gtk_button_new_with_label("Trek Connect Marketing Settings");
    gtk_box_pack_start(GTK_BOX(vbox), btn_create_quote, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), btn_process_sale, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), btn_trek_marketing, FALSE, FALSE, 0);

    g_signal_connect(btn_create_quote, "clicked", G_CALLBACK(create_quote_dialog), NULL);
    g_signal_connect(btn_process_sale, "clicked", G_CALLBACK(process_quote_to_sale_dialog), NULL);
    g_signal_connect(btn_trek_marketing, "clicked", G_CALLBACK(trek_marketing_settings_dialog), NULL);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void trek_marketing_settings_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];

    GtkWidget *dialog = create_dialog("Trek Connect Retail Marketing", GTK_WINDOW(main_window));
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *check_campaign = gtk_check_button_new_with_label("Enable Trek Campaign Emails");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_campaign), s->marketing.enable_campaign_emails);
    gtk_box_pack_start(GTK_BOX(vbox), check_campaign, FALSE, FALSE, 0);

    GtkWidget *check_post_purchase = gtk_check_button_new_with_label("Enable Post-Purchase Email Messaging");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_post_purchase), s->marketing.post_purchase_email_enabled);
    gtk_box_pack_start(GTK_BOX(vbox), check_post_purchase, FALSE, FALSE, 0);

    GtkWidget *entry_proof_email = create_labeled_entry("Proof Email Address:", vbox);
    gtk_entry_set_text(GTK_ENTRY(entry_proof_email), s->marketing.proof_email);

    GtkWidget *entry_facebook = create_labeled_entry("Facebook URL:", vbox);
    gtk_entry_set_text(GTK_ENTRY(entry_facebook), s->marketing.facebook_url);

    GtkWidget *entry_twitter = create_labeled_entry("Twitter URL:", vbox);
    gtk_entry_set_text(GTK_ENTRY(entry_twitter), s->marketing.twitter_url);

    GtkWidget *entry_survey = create_labeled_entry("Post-Purchase Survey URL:", vbox);
    gtk_entry_set_text(GTK_ENTRY(entry_survey), s->marketing.survey_url);

    GtkWidget *entry_post_message = create_labeled_entry("Post-Purchase Message:", vbox);
    gtk_entry_set_text(GTK_ENTRY(entry_post_message), s->marketing.post_purchase_message);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        s->marketing.enable_campaign_emails = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_campaign));
        s->marketing.post_purchase_email_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_post_purchase));
        strncpy(s->marketing.proof_email, gtk_entry_get_text(GTK_ENTRY(entry_proof_email)), NAME_LEN - 1);
        strncpy(s->marketing.facebook_url, gtk_entry_get_text(GTK_ENTRY(entry_facebook)), NAME_LEN - 1);
        strncpy(s->marketing.twitter_url, gtk_entry_get_text(GTK_ENTRY(entry_twitter)), NAME_LEN - 1);
        strncpy(s->marketing.survey_url, gtk_entry_get_text(GTK_ENTRY(entry_survey)), NAME_LEN - 1);
        strncpy(s->marketing.post_purchase_message, gtk_entry_get_text(GTK_ENTRY(entry_post_message)), NAME_LEN*2 - 1);

        show_info_dialog("Trek Connect Retail Marketing settings saved.");
    }

    gtk_widget_destroy(dialog);
}

static gboolean on_trend_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    Store *s = (Store *)user_data;
    if (!s || s->day_count == 0) {
        cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
        cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 14);
        cairo_move_to(cr, 50, 100);
        cairo_show_text(cr, "No sales data yet.");
        return FALSE;
    }

    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);

    // Draw background
    cairo_set_source_rgb(cr, 0.95, 0.95, 0.95);
    cairo_paint(cr);

    // Find max for scaling
    double max_sale = 0;
    for (int i = 0; i < s->day_count; i++) {
        if (s->daily_sales[i] > max_sale) max_sale = s->daily_sales[i];
    }
    if (max_sale == 0) max_sale = 100;

    // Chart parameters
    int margin_left = 80, margin_bottom = 60, margin_top = 40;
    int chart_width = width - margin_left - 30;
    int chart_height = height - margin_bottom - margin_top;
    int bar_width = chart_width / (s->day_count > 0 ? s->day_count : 1);
    if (bar_width < 2) bar_width = 2;

    // Draw title
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 16);
    cairo_move_to(cr, 50, 25);
    cairo_show_text(cr, "Daily Sales Trend");

    // Draw Y-axis label
    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12);
    cairo_move_to(cr, 10, margin_top + chart_height / 2);
    char y_label[32];
    sprintf(y_label, "Sales ($)");
    cairo_show_text(cr, y_label);

    // Draw axes
    cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
    cairo_set_line_width(cr, 2);
    // Y-axis
    cairo_move_to(cr, margin_left, margin_top);
    cairo_line_to(cr, margin_left, height - margin_bottom);
    // X-axis
    cairo_move_to(cr, margin_left, height - margin_bottom);
    cairo_line_to(cr, width - 20, height - margin_bottom);
    cairo_stroke(cr);

    // Draw grid lines and labels
    cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
    cairo_set_line_width(cr, 0.5);
    cairo_set_font_size(cr, 10);
    for (int i = 0; i <= 5; i++) {
        double y = margin_top + (chart_height * i / 5);
        double val = max_sale * (5 - i) / 5;
        // Grid line
        cairo_move_to(cr, margin_left - 5, y);
        cairo_line_to(cr, margin_left + chart_width, y);
        cairo_stroke(cr);
        // Y-axis label
        char label[32];
        sprintf(label, "$%.0f", val);
        cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
        cairo_move_to(cr, margin_left - 70, y + 4);
        cairo_show_text(cr, label);
    }

    // Draw bars
    cairo_set_source_rgb(cr, 0.2, 0.5, 0.8);
    for (int i = 0; i < s->day_count; i++) {
        double bar_height = (s->daily_sales[i] / max_sale) * chart_height;
        double x = margin_left + i * bar_width + 2;
        double y = height - margin_bottom - bar_height;

        cairo_rectangle(cr, x, y, bar_width - 4, bar_height);
        cairo_fill(cr);

        // Draw day label
        cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
        cairo_set_font_size(cr, 9);
        char day_label[16];
        sprintf(day_label, "D%d", i + 1);
        cairo_move_to(cr, x + bar_width / 4, height - margin_bottom + 15);
        cairo_show_text(cr, day_label);
    }

    return FALSE;
}

static void show_trend_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];

    if (s->day_count == 0) {
        show_error_dialog("No sales data available for this store.");
        return;
    }

    GtkWidget *dialog = create_dialog("Daily Sales Trend", GTK_WINDOW(main_window));
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *drawing_area = gtk_drawing_area_new();

    gtk_widget_set_size_request(drawing_area, 700, 400);
    gtk_container_add(GTK_CONTAINER(content_area), drawing_area);
    g_signal_connect(drawing_area, "draw", G_CALLBACK(on_trend_draw), s);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Save and load functions (same as original)
static void save_data(void) {
    FILE *f = fopen("ascend_data.txt", "w");
    if (!f) {
        show_error_dialog("Unable to save data!");
        return;
    }
    fprintf(f, "%d\n", store_count);
    for (int i = 0; i < store_count; i++) {
        Store *s = &stores[i];
        fprintf(f, "%s\n%s\n%.2f %.2f %d %d %d %d %d %d\n", s->name, s->location, s->monthly_goal, s->sales_to_date, s->transactions, s->product_count, s->day_count, s->quote_count, s->marketing.enable_campaign_emails, s->marketing.post_purchase_email_enabled);
        fprintf(f, "%s\n%s\n%s\n%s\n%s\n", s->marketing.proof_email, s->marketing.facebook_url, s->marketing.twitter_url, s->marketing.survey_url, s->marketing.post_purchase_message);
        for (int p = 0; p < s->product_count; p++) {
            Product *pr = &s->products[p];
            fprintf(f, "%s\n%s\n%s %d %.2f %d %d\n", pr->sku, pr->name, pr->vendor, pr->serialized, pr->price, pr->stock, pr->sold);
        }
        for (int d = 0; d < s->day_count; d++) fprintf(f, "%.2f\n", s->daily_sales[d]);
        for (int q = 0; q < s->quote_count; q++) {
            Quote *quote = &s->quotes[q];
            fprintf(f, "%s\n%s\n%d %.2f %d\n", quote->quote_id, quote->customer, quote->item_count, quote->total, quote->active);
            for (int k = 0; k < quote->item_count; k++) {
                fprintf(f, "%s %d\n", quote->item_sku[k], quote->qty[k]);
            }
        }
        // Save customers
        fprintf(f, "%d\n", s->customer_count);
        for (int c = 0; c < s->customer_count; c++) {
            Customer *cust = &s->customers[c];
            fprintf(f, "%s\n%s\n%s\n%d\n%s\n%s\n%s\n", cust->first_name, cust->last_name, cust->company, cust->account_type, cust->email, cust->phone1, cust->phone2);
            fprintf(f, "%s\n%s\n%s\n%s\n%s\n", cust->billing_addr1, cust->billing_addr2, cust->billing_city, cust->billing_state, cust->billing_zip);
            fprintf(f, "%s\n%s\n%s\n%s\n%s\n", cust->shipping_addr1, cust->shipping_addr2, cust->shipping_city, cust->shipping_state, cust->shipping_zip);
            fprintf(f, "%d\n%s\n%d %.2f %d %d\n%s\n", cust->use_billing_for_shipping, cust->tax_id, cust->poa_status, cust->credit_limit, cust->preferred_contact, cust->hidden, cust->notes);
        }
        // Save transactions
        fprintf(f, "%d\n", s->sales_count);
        for (int t = 0; t < s->sales_count; t++) {
            Transaction *txn = &s->sales[t];
            fprintf(f, "%s\n%d\n%d %.2f %.2f %d %d\n%s\n", txn->transaction_id, txn->customer_idx, txn->item_count, txn->total, txn->amount_paid, txn->payment_type, txn->status, txn->notes);
            fprintf(f, "%s\n%d\n", txn->date, txn->print_receipt);
                        fprintf(f, "%d\n%s\n", txn->is_return, txn->original_transaction_id);
            for (int k = 0; k < txn->item_count; k++) {
                fprintf(f, "%s %.2f %d\n", txn->item_sku[k], txn->item_price[k], txn->qty[k]);
            }
            // Save special order IDs and statuses for this transaction
            for (int k = 0; k < txn->item_count; k++) {
                fprintf(f, "%d %d\n", txn->special_order_id[k], txn->so_status[k]);
            }
        }
        // Save special orders
        fprintf(f, "%d\n", s->special_order_count);
        for (int so = 0; so < s->special_order_count; so++) {
            SpecialOrder *order = &s->special_orders[so];
            fprintf(f, "%d\n%s\n%d\n%d\n%s\n%d\n%s\n%s\n%s\n%s\n%s\n%d\n%d\n", 
                    order->special_order_id, order->product_sku, order->customer_idx, 
                    order->qty_ordered, order->order_date, order->status, order->po_number, 
                    order->comments, order->expected_date, order->received_date, 
                    order->notification_method, order->transfer_store_idx, order->notified);
        }
    }

    fprintf(f, "TAX_EXCEPTIONS\n%d\n", tax_exception_count);
    for (int i = 0; i < tax_exception_count; i++) {
        TaxExceptionReason *r = &tax_exceptions[i];
        fprintf(f, "%s\n%d\n%d\n", r->description, r->requires_tax_id, r->hidden);
    }

    fprintf(f, "TIME_CLOCK\n%d\n", time_clock_count);
    for (int i = 0; i < time_clock_count; i++) {
        TimeClockEntry *e = &time_clock_entries[i];
        fprintf(f, "%s\n%s\n%d\n%s\n%d\n",
                e->user_name,
                e->start_time,
                e->has_end_time,
                e->end_time,
                e->hidden);
    }

    fclose(f);
}

static void load_data(void) {
    FILE *f = fopen("ascend_data.txt", "r");
    if (!f) {
        show_error_dialog("Unable to load data!");
        return;
    }
    int cnt;
    if (fscanf(f, "%d\n", &cnt) != 1) cnt = 0;
    store_count = 0;
    for (int i = 0; i < cnt && i < MAX_STORES; i++) {
        Store *s = &stores[i];
        if (!fgets(s->name, NAME_LEN, f)) break;
        s->name[strcspn(s->name, "\n")] = '\0';
        if (!fgets(s->location, NAME_LEN, f)) break;
        s->location[strcspn(s->location, "\n")] = '\0';

        int rc = fscanf(f, "%lf %lf %d %d %d %d %d %d\n", &s->monthly_goal, &s->sales_to_date, &s->transactions, &s->product_count, &s->day_count, &s->quote_count, &s->marketing.enable_campaign_emails, &s->marketing.post_purchase_email_enabled);
        if (rc < 8) break;

        if (!fgets(s->marketing.proof_email, NAME_LEN, f)) break;
        s->marketing.proof_email[strcspn(s->marketing.proof_email, "\n")] = '\0';
        if (!fgets(s->marketing.facebook_url, NAME_LEN, f)) break;
        s->marketing.facebook_url[strcspn(s->marketing.facebook_url, "\n")] = '\0';
        if (!fgets(s->marketing.twitter_url, NAME_LEN, f)) break;
        s->marketing.twitter_url[strcspn(s->marketing.twitter_url, "\n")] = '\0';
        if (!fgets(s->marketing.survey_url, NAME_LEN, f)) break;
        s->marketing.survey_url[strcspn(s->marketing.survey_url, "\n")] = '\0';
        if (!fgets(s->marketing.post_purchase_message, NAME_LEN * 2, f)) break;
        s->marketing.post_purchase_message[strcspn(s->marketing.post_purchase_message, "\n")] = '\0';

        if (s->product_count > MAX_PRODUCTS) s->product_count = MAX_PRODUCTS;
        if (s->day_count > MAX_DAYS) s->day_count = MAX_DAYS;
        if (s->quote_count > MAX_QUOTES) s->quote_count = MAX_QUOTES;

        for (int p = 0; p < s->product_count; p++) {
            Product *pr = &s->products[p];
            if (!fgets(pr->sku, NAME_LEN, f)) break;
            pr->sku[strcspn(pr->sku, "\n")] = '\0';
            if (!fgets(pr->name, NAME_LEN, f)) break;
            pr->name[strcspn(pr->name, "\n")] = '\0';
            if (!fgets(pr->vendor, NAME_LEN, f)) break;
            pr->vendor[strcspn(pr->vendor, "\n")] = '\0';
            fscanf(f, "%d %lf %d %d\n", &pr->serialized, &pr->price, &pr->stock, &pr->sold);
        }
        for (int d = 0; d < s->day_count; d++) fscanf(f, "%lf\n", &s->daily_sales[d]);

        for (int q = 0; q < s->quote_count; q++) {
            Quote *quote = &s->quotes[q];
            if (!fgets(quote->quote_id, NAME_LEN, f)) break;
            quote->quote_id[strcspn(quote->quote_id, "\n")] = '\0';
            if (!fgets(quote->customer, NAME_LEN, f)) break;
            quote->customer[strcspn(quote->customer, "\n")] = '\0';
            fscanf(f, "%d %lf %d\n", &quote->item_count, &quote->total, &quote->active);
            if (quote->item_count > MAX_PRODUCTS) quote->item_count = MAX_PRODUCTS;
            for (int k = 0; k < quote->item_count; k++) {
                fscanf(f, "%s %d\n", quote->item_sku[k], &quote->qty[k]);
                quote->register_status[k] = 0;
                strcpy(quote->postpone_date[k], "");
            }
        }

        // Load customers
        int cust_cnt = 0;
        if (fscanf(f, "%d\n", &cust_cnt) == 1) {
            s->customer_count = (cust_cnt < MAX_CUSTOMERS) ? cust_cnt : MAX_CUSTOMERS;
            for (int c = 0; c < s->customer_count; c++) {
                Customer *cust = &s->customers[c];
                if (!fgets(cust->first_name, NAME_LEN, f)) break;
                cust->first_name[strcspn(cust->first_name, "\n")] = '\0';
                if (!fgets(cust->last_name, NAME_LEN, f)) break;
                cust->last_name[strcspn(cust->last_name, "\n")] = '\0';
                if (!fgets(cust->company, NAME_LEN, f)) break;
                cust->company[strcspn(cust->company, "\n")] = '\0';
                fscanf(f, "%d\n", (int *)&cust->account_type);
                if (!fgets(cust->email, NAME_LEN, f)) break;
                cust->email[strcspn(cust->email, "\n")] = '\0';
                if (!fgets(cust->phone1, NAME_LEN, f)) break;
                cust->phone1[strcspn(cust->phone1, "\n")] = '\0';
                if (!fgets(cust->phone2, NAME_LEN, f)) break;
                cust->phone2[strcspn(cust->phone2, "\n")] = '\0';
                if (!fgets(cust->billing_addr1, NAME_LEN, f)) break;
                cust->billing_addr1[strcspn(cust->billing_addr1, "\n")] = '\0';
                if (!fgets(cust->billing_addr2, NAME_LEN, f)) break;
                cust->billing_addr2[strcspn(cust->billing_addr2, "\n")] = '\0';
                if (!fgets(cust->billing_city, NAME_LEN, f)) break;
                cust->billing_city[strcspn(cust->billing_city, "\n")] = '\0';
                if (!fgets(cust->billing_state, NAME_LEN, f)) break;
                cust->billing_state[strcspn(cust->billing_state, "\n")] = '\0';
                if (!fgets(cust->billing_zip, NAME_LEN, f)) break;
                cust->billing_zip[strcspn(cust->billing_zip, "\n")] = '\0';
                if (!fgets(cust->shipping_addr1, NAME_LEN, f)) break;
                cust->shipping_addr1[strcspn(cust->shipping_addr1, "\n")] = '\0';
                if (!fgets(cust->shipping_addr2, NAME_LEN, f)) break;
                cust->shipping_addr2[strcspn(cust->shipping_addr2, "\n")] = '\0';
                if (!fgets(cust->shipping_city, NAME_LEN, f)) break;
                cust->shipping_city[strcspn(cust->shipping_city, "\n")] = '\0';
                if (!fgets(cust->shipping_state, NAME_LEN, f)) break;
                cust->shipping_state[strcspn(cust->shipping_state, "\n")] = '\0';
                if (!fgets(cust->shipping_zip, NAME_LEN, f)) break;
                cust->shipping_zip[strcspn(cust->shipping_zip, "\n")] = '\0';
                fscanf(f, "%d\n", &cust->use_billing_for_shipping);
                if (!fgets(cust->tax_id, NAME_LEN, f)) break;
                cust->tax_id[strcspn(cust->tax_id, "\n")] = '\0';
                fscanf(f, "%d %lf %d %d\n", &cust->poa_status, &cust->credit_limit, &cust->preferred_contact, &cust->hidden);
                if (!fgets(cust->notes, NAME_LEN * 2, f)) break;
                cust->notes[strcspn(cust->notes, "\n")] = '\0';
            }
        } else {
            s->customer_count = 0;
        }

        // Load transactions
        int txn_cnt = 0;
        if (fscanf(f, "%d\n", &txn_cnt) == 1) {
            s->sales_count = (txn_cnt < MAX_TRANSACTIONS) ? txn_cnt : MAX_TRANSACTIONS;
            for (int t = 0; t < s->sales_count; t++) {
                Transaction *txn = &s->sales[t];
                if (!fgets(txn->transaction_id, NAME_LEN, f)) break;
                txn->transaction_id[strcspn(txn->transaction_id, "\n")] = '\0';
                fscanf(f, "%d\n%d %lf %lf %d %d\n", &txn->customer_idx, &txn->item_count, &txn->total, &txn->amount_paid, &txn->payment_type, &txn->status);
                if (!fgets(txn->notes, NAME_LEN * 2, f)) break;
                txn->notes[strcspn(txn->notes, "\n")] = '\0';
                if (!fgets(txn->date, NAME_LEN, f)) break;
                txn->date[strcspn(txn->date, "\n")] = '\0';
                fscanf(f, "%d\n", &txn->print_receipt);
                                fscanf(f, "%d\n", &txn->is_return);
                                if (!fgets(txn->original_transaction_id, NAME_LEN, f)) break;
                                txn->original_transaction_id[strcspn(txn->original_transaction_id, "\n")] = '\0';
                if (txn->item_count > MAX_PRODUCTS) txn->item_count = MAX_PRODUCTS;
                for (int k = 0; k < txn->item_count; k++) {
                    fscanf(f, "%s %lf %d\n", txn->item_sku[k], &txn->item_price[k], &txn->qty[k]);
                }
                // Load special order IDs and statuses for this transaction
                for (int k = 0; k < txn->item_count; k++) {
                    fscanf(f, "%d %d\n", &txn->special_order_id[k], &txn->so_status[k]);
                }
            }
        } else {
            s->sales_count = 0;
        }

        // Load special orders
        int so_cnt = 0;
        if (fscanf(f, "%d\n", &so_cnt) == 1) {
            s->special_order_count = (so_cnt < MAX_SPECIAL_ORDERS) ? so_cnt : MAX_SPECIAL_ORDERS;
            for (int so = 0; so < s->special_order_count; so++) {
                SpecialOrder *order = &s->special_orders[so];
                fscanf(f, "%d\n", &order->special_order_id);
                if (!fgets(order->product_sku, NAME_LEN, f)) break;
                order->product_sku[strcspn(order->product_sku, "\n")] = '\0';
                fscanf(f, "%d\n%d\n%s\n%d\n", &order->customer_idx, &order->qty_ordered, order->order_date, &order->status);
                if (!fgets(order->po_number, NAME_LEN, f)) break;
                order->po_number[strcspn(order->po_number, "\n")] = '\0';
                if (!fgets(order->comments, NAME_LEN * 2, f)) break;
                order->comments[strcspn(order->comments, "\n")] = '\0';
                if (!fgets(order->expected_date, NAME_LEN, f)) break;
                order->expected_date[strcspn(order->expected_date, "\n")] = '\0';
                if (!fgets(order->received_date, NAME_LEN, f)) break;
                order->received_date[strcspn(order->received_date, "\n")] = '\0';
                if (!fgets(order->notification_method, NAME_LEN, f)) break;
                order->notification_method[strcspn(order->notification_method, "\n")] = '\0';
                fscanf(f, "%d\n%d\n", &order->transfer_store_idx, &order->notified);
            }
        } else {
            s->special_order_count = 0;
        }

        store_count++;
    }

    tax_exception_count = 0;
    time_clock_count = 0;

    char section_name[NAME_LEN];
    while (fscanf(f, "%63s", section_name) == 1) {
        if (strcmp(section_name, "TAX_EXCEPTIONS") == 0) {
            int count = 0;
            if (fscanf(f, "%d\n", &count) != 1) break;
            int limit = (count < MAX_TAX_EXCEPTIONS) ? count : MAX_TAX_EXCEPTIONS;
            tax_exception_count = limit;
            for (int i = 0; i < count; i++) {
                char desc_buf[NAME_LEN * 2];
                int req_tax = 0;
                int hidden = 0;
                if (!fgets(desc_buf, sizeof(desc_buf), f)) break;
                desc_buf[strcspn(desc_buf, "\n")] = '\0';
                if (fscanf(f, "%d\n%d\n", &req_tax, &hidden) != 2) break;

                if (i < limit) {
                    TaxExceptionReason *r = &tax_exceptions[i];
                    strncpy(r->description, desc_buf, sizeof(r->description) - 1);
                    r->description[sizeof(r->description) - 1] = '\0';
                    r->requires_tax_id = req_tax;
                    r->hidden = hidden;
                }
            }
        } else if (strcmp(section_name, "TIME_CLOCK") == 0) {
            int count = 0;
            if (fscanf(f, "%d\n", &count) != 1) break;
            int limit = (count < MAX_TIME_CLOCK_ENTRIES) ? count : MAX_TIME_CLOCK_ENTRIES;
            time_clock_count = limit;
            for (int i = 0; i < count; i++) {
                char user_buf[NAME_LEN];
                char start_buf[NAME_LEN];
                int has_end = 0;
                char end_buf[NAME_LEN];
                int hidden = 0;

                if (!fgets(user_buf, sizeof(user_buf), f)) break;
                user_buf[strcspn(user_buf, "\n")] = '\0';
                if (!fgets(start_buf, sizeof(start_buf), f)) break;
                start_buf[strcspn(start_buf, "\n")] = '\0';
                if (fscanf(f, "%d\n", &has_end) != 1) break;
                if (!fgets(end_buf, sizeof(end_buf), f)) break;
                end_buf[strcspn(end_buf, "\n")] = '\0';
                if (fscanf(f, "%d\n", &hidden) != 1) break;

                if (i < limit) {
                    TimeClockEntry *e = &time_clock_entries[i];
                    strncpy(e->user_name, user_buf, NAME_LEN - 1);
                    e->user_name[NAME_LEN - 1] = '\0';
                    strncpy(e->start_time, start_buf, NAME_LEN - 1);
                    e->start_time[NAME_LEN - 1] = '\0';
                    e->has_end_time = has_end;
                    strncpy(e->end_time, end_buf, NAME_LEN - 1);
                    e->end_time[NAME_LEN - 1] = '\0';
                    e->hidden = hidden;
                }
            }
        } else {
            break;
        }
    }

    fclose(f);
}

static void add_customer_dialog(Store *s) {
    if (!s) return;
    if (s->customer_count >= MAX_CUSTOMERS) {
        show_error_dialog("Maximum customers reached for this store.");
        return;
    }

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Add Customer",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Save", 1,
                                                   "_Cancel", 2,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(content_area), scroll);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), vbox);

    // Account Type
    GtkWidget *type_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), type_hbox, FALSE, FALSE, 0);
    GtkWidget *type_label = gtk_label_new("Account Type:");
    gtk_box_pack_start(GTK_BOX(type_hbox), type_label, FALSE, FALSE, 0);
    GtkWidget *type_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_combo), "Individual");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_combo), "Company");
    gtk_combo_box_set_active(GTK_COMBO_BOX(type_combo), 0);
    gtk_box_pack_start(GTK_BOX(type_hbox), type_combo, FALSE, FALSE, 0);

    // Name fields
    GtkWidget *first_entry = create_labeled_entry("First Name:", vbox);
    GtkWidget *last_entry = create_labeled_entry("Last Name:", vbox);
    GtkWidget *company_entry = create_labeled_entry("Company:", vbox);

    // Contact info
    GtkWidget *email_entry = create_labeled_entry("Email:", vbox);
    GtkWidget *phone1_entry = create_labeled_entry("Primary Phone:", vbox);
    GtkWidget *phone2_entry = create_labeled_entry("Secondary Phone:", vbox);

    // Preferred contact
    GtkWidget *pref_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), pref_hbox, FALSE, FALSE, 0);
    GtkWidget *pref_label = gtk_label_new("Preferred Contact:");
    gtk_box_pack_start(GTK_BOX(pref_hbox), pref_label, FALSE, FALSE, 0);
    GtkWidget *pref_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pref_combo), "Email");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pref_combo), "Phone 1");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pref_combo), "Phone 2");
    gtk_combo_box_set_active(GTK_COMBO_BOX(pref_combo), 0);
    gtk_box_pack_start(GTK_BOX(pref_hbox), pref_combo, FALSE, FALSE, 0);

    // Billing Address
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("BILLING ADDRESS"), FALSE, FALSE, 5);
    GtkWidget *bill_addr1 = create_labeled_entry("Address 1:", vbox);
    GtkWidget *bill_addr2 = create_labeled_entry("Address 2:", vbox);
    GtkWidget *bill_city = create_labeled_entry("City:", vbox);
    GtkWidget *bill_state = create_labeled_entry("State:", vbox);
    GtkWidget *bill_zip = create_labeled_entry("ZIP:", vbox);

    // Shipping Address checkbox
    GtkWidget *use_billing_cb = gtk_check_button_new_with_label("Use billing address for shipping");
    gtk_box_pack_start(GTK_BOX(vbox), use_billing_cb, FALSE, FALSE, 0);

    // Shipping Address
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("SHIPPING ADDRESS"), FALSE, FALSE, 5);
    GtkWidget *ship_addr1 = create_labeled_entry("Address 1:", vbox);
    GtkWidget *ship_addr2 = create_labeled_entry("Address 2:", vbox);
    GtkWidget *ship_city = create_labeled_entry("City:", vbox);
    GtkWidget *ship_state = create_labeled_entry("State:", vbox);
    GtkWidget *ship_zip = create_labeled_entry("ZIP:", vbox);

    // Tax & Credit
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("TAX & CREDIT"), FALSE, FALSE, 5);
    GtkWidget *tax_id_entry = create_labeled_entry("Tax ID:", vbox);
    GtkWidget *credit_entry = create_labeled_entry("Credit Limit:", vbox);

    // POA Status
    GtkWidget *poa_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), poa_hbox, FALSE, FALSE, 0);
    GtkWidget *poa_label = gtk_label_new("POA Status:");
    gtk_box_pack_start(GTK_BOX(poa_hbox), poa_label, FALSE, FALSE, 0);
    GtkWidget *poa_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(poa_combo), "Inactive");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(poa_combo), "Active");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(poa_combo), "Suspended");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(poa_combo), "Closed");
    gtk_combo_box_set_active(GTK_COMBO_BOX(poa_combo), 0);
    gtk_box_pack_start(GTK_BOX(poa_hbox), poa_combo, FALSE, FALSE, 0);

    // Notes
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("NOTES"), FALSE, FALSE, 5);
    GtkWidget *notes_sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(notes_sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    GtkWidget *notes_view = gtk_text_view_new();
    gtk_container_add(GTK_CONTAINER(notes_sw), notes_view);
    gtk_box_pack_start(GTK_BOX(vbox), notes_sw, TRUE, TRUE, 0);

    gtk_widget_set_size_request(dialog, 500, 600);
    gtk_widget_show_all(dialog);

    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == 1) {
        Customer *c = &s->customers[s->customer_count];
        c->account_type = gtk_combo_box_get_active(GTK_COMBO_BOX(type_combo));
        strncpy(c->first_name, gtk_entry_get_text(GTK_ENTRY(first_entry)), NAME_LEN - 1);
        strncpy(c->last_name, gtk_entry_get_text(GTK_ENTRY(last_entry)), NAME_LEN - 1);
        strncpy(c->company, gtk_entry_get_text(GTK_ENTRY(company_entry)), NAME_LEN - 1);
        strncpy(c->email, gtk_entry_get_text(GTK_ENTRY(email_entry)), NAME_LEN - 1);
        strncpy(c->phone1, gtk_entry_get_text(GTK_ENTRY(phone1_entry)), NAME_LEN - 1);
        strncpy(c->phone2, gtk_entry_get_text(GTK_ENTRY(phone2_entry)), NAME_LEN - 1);
        c->preferred_contact = gtk_combo_box_get_active(GTK_COMBO_BOX(pref_combo));

        strncpy(c->billing_addr1, gtk_entry_get_text(GTK_ENTRY(bill_addr1)), NAME_LEN - 1);
        strncpy(c->billing_addr2, gtk_entry_get_text(GTK_ENTRY(bill_addr2)), NAME_LEN - 1);
        strncpy(c->billing_city, gtk_entry_get_text(GTK_ENTRY(bill_city)), NAME_LEN - 1);
        strncpy(c->billing_state, gtk_entry_get_text(GTK_ENTRY(bill_state)), NAME_LEN - 1);
        strncpy(c->billing_zip, gtk_entry_get_text(GTK_ENTRY(bill_zip)), NAME_LEN - 1);

        c->use_billing_for_shipping = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(use_billing_cb));
        strncpy(c->shipping_addr1, gtk_entry_get_text(GTK_ENTRY(ship_addr1)), NAME_LEN - 1);
        strncpy(c->shipping_addr2, gtk_entry_get_text(GTK_ENTRY(ship_addr2)), NAME_LEN - 1);
        strncpy(c->shipping_city, gtk_entry_get_text(GTK_ENTRY(ship_city)), NAME_LEN - 1);
        strncpy(c->shipping_state, gtk_entry_get_text(GTK_ENTRY(ship_state)), NAME_LEN - 1);
        strncpy(c->shipping_zip, gtk_entry_get_text(GTK_ENTRY(ship_zip)), NAME_LEN - 1);

        strncpy(c->tax_id, gtk_entry_get_text(GTK_ENTRY(tax_id_entry)), NAME_LEN - 1);
        c->credit_limit = atof(gtk_entry_get_text(GTK_ENTRY(credit_entry)));
        c->poa_status = gtk_combo_box_get_active(GTK_COMBO_BOX(poa_combo));

        GtkTextIter start, end;
        GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(notes_view));
        gtk_text_buffer_get_bounds(buf, &start, &end);
        char *notes = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
        strncpy(c->notes, notes, NAME_LEN * 2 - 1);
        g_free(notes);

        c->hidden = 0;
        s->customer_count++;
        show_info_dialog("Customer added successfully.");
    }

    gtk_widget_destroy(dialog);
}

static void list_customers_dialog(Store *s) {
    if (!s) return;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Manage Customers",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Add", 1,
                                                   "_Close", 2,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    // Search bar
    GtkWidget *search_entry = create_labeled_entry("Search by name/company:", vbox);

    // Customer list
    GtkWidget *list_sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(list_sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), list_sw, TRUE, TRUE, 0);

    GtkListStore *store_list = gtk_list_store_new(6,
                                                   G_TYPE_INT,     // customer index
                                                   G_TYPE_STRING,  // name
                                                   G_TYPE_STRING,  // company
                                                   G_TYPE_STRING,  // email
                                                   G_TYPE_STRING,  // poa status
                                                   G_TYPE_STRING); // credit limit

    GtkTreeView *tree = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_list)));
    
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Name", renderer, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Company", renderer, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Email", renderer, "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "POA Status", renderer, "text", 4, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Credit Limit", renderer, "text", 5, NULL);

    // Populate list
    for (int i = 0; i < s->customer_count; i++) {
        if (s->customers[i].hidden) continue; // Skip hidden customers
        Customer *c = &s->customers[i];
        char name[NAME_LEN * 2];
        sprintf(name, "%s %s", c->first_name, c->last_name);
        const char *poa_names[] = {"Inactive", "Active", "Suspended", "Closed"};
        char credit[30];
        sprintf(credit, "%.2f", c->credit_limit);

        GtkTreeIter iter;
        gtk_list_store_append(store_list, &iter);
        gtk_list_store_set(store_list, &iter,
                          0, i,
                          1, name,
                          2, c->company,
                          3, c->email,
                          4, poa_names[c->poa_status],
                          5, credit,
                          -1);
    }

    gtk_container_add(GTK_CONTAINER(list_sw), GTK_WIDGET(tree));

    gtk_widget_set_size_request(dialog, 700, 400);
    gtk_widget_show_all(dialog);

    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == 1) {
        add_customer_dialog(s);
    }

    gtk_widget_destroy(dialog);
}

static void manage_customers_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    list_customers_dialog(&stores[si]);
}

static void create_sale_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];

    if (s->customer_count == 0) {
        show_error_dialog("No customers in this store. Add a customer first.");
        return;
    }

    if (s->sales_count >= MAX_TRANSACTIONS) {
        show_error_dialog("Maximum transactions reached for this store.");
        return;
    }

    // Initialize new transaction
    Transaction txn;
    memset(&txn, 0, sizeof(Transaction));
    txn.item_count = 0;
    txn.total = 0;
    txn.amount_paid = 0;
    txn.status = 0; // open
    txn.customer_idx = -1; // none selected yet
    sprintf(txn.transaction_id, "TXN-%d-%ld", si, (long)time(NULL) % 10000);

    // Select Customer
    GtkWidget *cust_dialog = gtk_dialog_new_with_buttons("Select Customer",
                                                         GTK_WINDOW(main_window),
                                                         GTK_DIALOG_MODAL,
                                                         "_OK", GTK_RESPONSE_OK,
                                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                                         NULL);
    GtkWidget *cust_area = gtk_dialog_get_content_area(GTK_DIALOG(cust_dialog));
    GtkWidget *cust_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(cust_area), cust_box);

    GtkWidget *search_entry = create_labeled_entry("Search (name/company):", cust_box);
    GtkWidget *cust_combo = gtk_combo_box_text_new();

    // Populate customer list
    for (int i = 0; i < s->customer_count; i++) {
        if (s->customers[i].hidden) continue;
        Customer *c = &s->customers[i];
        char entry[NAME_LEN * 2];
        sprintf(entry, "%s %s (%s)", c->first_name, c->last_name, c->company);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cust_combo), entry);
    }

    gtk_box_pack_start(GTK_BOX(cust_box), gtk_label_new("Select Customer:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cust_box), cust_combo, FALSE, FALSE, 0);
    gtk_widget_show_all(cust_dialog);

    if (gtk_dialog_run(GTK_DIALOG(cust_dialog)) != GTK_RESPONSE_OK) {
        gtk_widget_destroy(cust_dialog);
        return;
    }

    int cust_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(cust_combo));
    if (cust_idx >= 0 && cust_idx < s->customer_count) {
        txn.customer_idx = cust_idx;
    }
    gtk_widget_destroy(cust_dialog);

    // Main transaction dialog
    GtkWidget *txn_dialog = gtk_dialog_new_with_buttons("Transaction Editor",
                                                        GTK_WINDOW(main_window),
                                                        GTK_DIALOG_MODAL,
                                                        "_Add Item", 1,
                                                        "_Take Payment", 2,
                                                        "_Cancel", GTK_RESPONSE_CANCEL,
                                                        NULL);
    GtkWidget *txn_area = gtk_dialog_get_content_area(GTK_DIALOG(txn_dialog));
    GtkWidget *txn_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(txn_area), txn_box);

    // Customer info header
    char cust_info[NAME_LEN * 3];
    if (txn.customer_idx >= 0) {
        Customer *c = &s->customers[txn.customer_idx];
        sprintf(cust_info, "Customer: %s %s (%s)", c->first_name, c->last_name, c->company);
    } else {
        sprintf(cust_info, "Customer: Not selected");
    }
    gtk_box_pack_start(GTK_BOX(txn_box), gtk_label_new(cust_info), FALSE, FALSE, 0);

    // Item list display
    GtkWidget *item_sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(item_sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    GtkWidget *item_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(item_view), FALSE);
    gtk_container_add(GTK_CONTAINER(item_sw), item_view);
    gtk_box_pack_start(GTK_BOX(txn_box), item_sw, TRUE, TRUE, 0);

    GtkWidget *total_label = gtk_label_new("Total: $0.00");
    gtk_box_pack_start(GTK_BOX(txn_box), total_label, FALSE, FALSE, 0);

    gtk_widget_set_size_request(txn_dialog, 500, 400);
    gtk_widget_show_all(txn_dialog);

    // Transaction editing loop
    int done = 0;
    while (!done) {
        // Update item list display
        GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(item_view));
        char item_text[2000] = "ITEMS:\n\n";
        for (int i = 0; i < txn.item_count; i++) {
            Product *p = NULL;
            for (int j = 0; j < s->product_count; j++) {
                if (strcmp(s->products[j].sku, txn.item_sku[i]) == 0) {
                    p = &s->products[j];
                    break;
                }
            }
            if (p) {
                char line[200];
                sprintf(line, "%d. %s (SKU: %s) Qty: %d @ $%.2f = $%.2f\n",
                        i + 1, p->name, p->sku, txn.qty[i], txn.item_price[i],
                        txn.item_price[i] * txn.qty[i]);
                strcat(item_text, line);
            }
        }
        gtk_text_buffer_set_text(buf, item_text, -1);

        char total_str[100];
        sprintf(total_str, "Total: $%.2f", txn.total);
        gtk_label_set_text(GTK_LABEL(total_label), total_str);

        int response = gtk_dialog_run(GTK_DIALOG(txn_dialog));

        if (response == 1) { // Add Item
            // Product search dialog
            GtkWidget *prod_dialog = gtk_dialog_new_with_buttons("Add Product to Sale",
                                                                 GTK_WINDOW(main_window),
                                                                 GTK_DIALOG_MODAL,
                                                                 "_Add", GTK_RESPONSE_OK,
                                                                 "_Cancel", GTK_RESPONSE_CANCEL,
                                                                 NULL);
            GtkWidget *prod_area = gtk_dialog_get_content_area(GTK_DIALOG(prod_dialog));
            GtkWidget *prod_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
            gtk_container_add(GTK_CONTAINER(prod_area), prod_box);

            GtkWidget *sku_entry = create_labeled_entry("SKU/UPC:", prod_box);
            GtkWidget *qty_entry = create_labeled_entry("Quantity:", prod_box);
            gtk_entry_set_text(GTK_ENTRY(qty_entry), "1");

            gtk_widget_show_all(prod_dialog);

            if (gtk_dialog_run(GTK_DIALOG(prod_dialog)) == GTK_RESPONSE_OK) {
                const char *sku = gtk_entry_get_text(GTK_ENTRY(sku_entry));
                const char *qty_str = gtk_entry_get_text(GTK_ENTRY(qty_entry));

                Product *p = NULL;
                for (int j = 0; j < s->product_count; j++) {
                    if (strcmp(s->products[j].sku, sku) == 0) {
                        p = &s->products[j];
                        break;
                    }
                }

                if (p && txn.item_count < MAX_PRODUCTS) {
                    int qty = atoi(qty_str);
                    int is_special_order = 0;
                    char so_comments[NAME_LEN] = "";

                    // Check if out of stock
                    if (p->stock < qty) {
                        special_order_prompt_dialog(s, sku, qty, &is_special_order, so_comments);
                    }

                    strncpy(txn.item_sku[txn.item_count], sku, NAME_LEN - 1);
                    txn.item_price[txn.item_count] = p->price;
                    txn.qty[txn.item_count] = qty;
                    txn.is_special_order[txn.item_count] = is_special_order;
                    if (is_special_order) {
                        strncpy(txn.so_comments[txn.item_count], so_comments, NAME_LEN - 1);
                    }
                    txn.total += p->price * qty;
                    txn.item_count++;

                    // Create special order record if needed
                    if (is_special_order) {
                        SpecialOrder so;
                        memset(&so, 0, sizeof(SpecialOrder));
                        so.special_order_id = s->special_order_count;
                        so.transaction_idx = s->sales_count; // Will be set when transaction is saved
                        so.store_idx = si;
                        so.customer_idx = txn.customer_idx;
                        strncpy(so.product_sku, sku, NAME_LEN - 1);
                        so.qty_ordered = qty;
                        so.status = SO_STATUS_NOT_ORDERED;
                        time_t now = time(NULL);
                        strftime(so.order_date, sizeof(so.order_date), "%Y-%m-%d", localtime(&now));
                        strncpy(so.comments, so_comments, NAME_LEN * 2 - 1);

                        if (s->special_order_count < MAX_SPECIAL_ORDERS) {
                            s->special_orders[s->special_order_count++] = so;
                        }
                    }
                } else if (!p) {
                    show_error_dialog("Product not found. Check SKU/UPC.");
                }
            }
            gtk_widget_destroy(prod_dialog);
        } else if (response == 2) { // Take Payment
            done = 1;
        } else {
            // Cancel
            gtk_widget_destroy(txn_dialog);
            return;
        }
    }

    gtk_widget_destroy(txn_dialog);

    // Payment dialog
    GtkWidget *pay_dialog = gtk_dialog_new_with_buttons("Take Payment",
                                                        GTK_WINDOW(main_window),
                                                        GTK_DIALOG_MODAL,
                                                        "_Complete Sale", 1,
                                                        "_Cancel", GTK_RESPONSE_CANCEL,
                                                        NULL);
    GtkWidget *pay_area = gtk_dialog_get_content_area(GTK_DIALOG(pay_dialog));
    GtkWidget *pay_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(pay_area), pay_box);

    char sale_total[100];
    sprintf(sale_total, "Sale Total: $%.2f", txn.total);
    gtk_box_pack_start(GTK_BOX(pay_box), gtk_label_new(sale_total), FALSE, FALSE, 0);

    GtkWidget *payment_type_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(payment_type_combo), "Cash");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(payment_type_combo), "Credit Card");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(payment_type_combo), "Debit Card");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(payment_type_combo), "Gift Card");
    gtk_combo_box_set_active(GTK_COMBO_BOX(payment_type_combo), 0);
    gtk_box_pack_start(GTK_BOX(pay_box), gtk_label_new("Payment Type:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pay_box), payment_type_combo, FALSE, FALSE, 0);

    GtkWidget *amount_entry = create_labeled_entry("Amount Paid:", pay_box);
    char total_str[50];
    sprintf(total_str, "%.2f", txn.total);
    gtk_entry_set_text(GTK_ENTRY(amount_entry), total_str);

    GtkWidget *receipt_checkbox = gtk_check_button_new_with_label("Print Receipt");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(receipt_checkbox), TRUE);
    gtk_box_pack_start(GTK_BOX(pay_box), receipt_checkbox, FALSE, FALSE, 0);

    GtkWidget *keep_open_checkbox = gtk_check_button_new_with_label("Keep transaction open");
    gtk_box_pack_start(GTK_BOX(pay_box), keep_open_checkbox, FALSE, FALSE, 0);

    gtk_widget_show_all(pay_dialog);

    if (gtk_dialog_run(GTK_DIALOG(pay_dialog)) == 1) {
        txn.amount_paid = atof(gtk_entry_get_text(GTK_ENTRY(amount_entry)));
        txn.payment_type = gtk_combo_box_get_active(GTK_COMBO_BOX(payment_type_combo));
        txn.print_receipt = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(receipt_checkbox));
        int keep_open = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(keep_open_checkbox));

        if (txn.amount_paid >= txn.total) {
            if (keep_open) {
                txn.status = 0; // keep open even when paid in full
            } else {
                txn.status = 2; // paid in full
                s->sales_to_date += txn.total;
            }
            s->sales_count++;
            s->sales[s->sales_count - 1] = txn;
            s->transactions++;

            char change_str[100];
            double change = txn.amount_paid - txn.total;
            sprintf(change_str, "Sale Completed!\n\nTotal: $%.2f\nPaid: $%.2f\nChange: $%.2f",
                    txn.total, txn.amount_paid, change);
            show_info_dialog(change_str);

            if (txn.print_receipt) {
                // Generate and show receipt
                char receipt[1000] = "";
                strcat(receipt, "===== RECEIPT =====\n\n");
                sprintf(receipt + strlen(receipt), "Transaction ID: %s\n", txn.transaction_id);
                if (txn.customer_idx >= 0) {
                    Customer *c = &s->customers[txn.customer_idx];
                    sprintf(receipt + strlen(receipt), "Customer: %s %s\n", c->first_name, c->last_name);
                }
                strcat(receipt, "\nITEMS:\n");
                for (int i = 0; i < txn.item_count; i++) {
                    Product *p = NULL;
                    for (int j = 0; j < s->product_count; j++) {
                        if (strcmp(s->products[j].sku, txn.item_sku[i]) == 0) {
                            p = &s->products[j];
                            break;
                        }
                    }
                    if (p) {
                        sprintf(receipt + strlen(receipt), "%s x%d @ $%.2f = $%.2f\n",
                                p->name, txn.qty[i], txn.item_price[i],
                                txn.item_price[i] * txn.qty[i]);
                    }
                }
                sprintf(receipt + strlen(receipt), "\nTotal: $%.2f\nPaid: $%.2f\nChange: $%.2f\n\n",
                        txn.total, txn.amount_paid, change);
                strcat(receipt, "Thank you for your purchase!");

                show_info_dialog(receipt);
            }
        } else {
            txn.status = 0; // open/layaway
            s->sales_count++;
            s->sales[s->sales_count - 1] = txn;

            char msg[150];
            double remaining = txn.total - txn.amount_paid;
            sprintf(msg, "Layaway Transaction Created\n\nTotal: $%.2f\nPaid: $%.2f\nRemaining: $%.2f",
                    txn.total, txn.amount_paid, remaining);
            show_info_dialog(msg);
        }
    }

    gtk_widget_destroy(pay_dialog);
}

static void create_layaway_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];

    if (s->customer_count == 0) {
        show_error_dialog("No customers in this store. Add a customer first.");
        return;
    }

    if (s->sales_count >= MAX_TRANSACTIONS) {
        show_error_dialog("Maximum transactions reached for this store.");
        return;
    }

    // Initialize new layaway transaction
    Transaction txn;
    memset(&txn, 0, sizeof(Transaction));
    txn.item_count = 0;
    txn.total = 0;
    txn.amount_paid = 0;
    txn.status = 0; // open layaway (automatically set)
    txn.customer_idx = -1; // none selected yet
    sprintf(txn.transaction_id, "LAY-%d-%ld", si, (long)time(NULL) % 10000);

    // Select Customer
    GtkWidget *cust_dialog = gtk_dialog_new_with_buttons("Select Customer for Layaway",
                                                         GTK_WINDOW(main_window),
                                                         GTK_DIALOG_MODAL,
                                                         "_OK", GTK_RESPONSE_OK,
                                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                                         NULL);
    GtkWidget *cust_area = gtk_dialog_get_content_area(GTK_DIALOG(cust_dialog));
    GtkWidget *cust_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(cust_area), cust_box);

    GtkWidget *search_entry = create_labeled_entry("Search (name/company):", cust_box);
    GtkWidget *cust_combo = gtk_combo_box_text_new();

    // Populate customer list
    for (int i = 0; i < s->customer_count; i++) {
        if (s->customers[i].hidden) continue;
        Customer *c = &s->customers[i];
        char entry[NAME_LEN * 2];
        sprintf(entry, "%s %s (%s)", c->first_name, c->last_name, c->company);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cust_combo), entry);
    }

    gtk_box_pack_start(GTK_BOX(cust_box), gtk_label_new("Select Customer:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cust_box), cust_combo, FALSE, FALSE, 0);
    gtk_widget_show_all(cust_dialog);

    if (gtk_dialog_run(GTK_DIALOG(cust_dialog)) != GTK_RESPONSE_OK) {
        gtk_widget_destroy(cust_dialog);
        return;
    }

    int cust_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(cust_combo));
    if (cust_idx >= 0 && cust_idx < s->customer_count) {
        txn.customer_idx = cust_idx;
    }
    gtk_widget_destroy(cust_dialog);

    // Main layaway transaction dialog
    GtkWidget *txn_dialog = gtk_dialog_new_with_buttons("Layaway Transaction Editor",
                                                        GTK_WINDOW(main_window),
                                                        GTK_DIALOG_MODAL,
                                                        "_Add Item", 1,
                                                        "_Take Payment", 2,
                                                        "_Save Layaway", 3,
                                                        "_Cancel", GTK_RESPONSE_CANCEL,
                                                        NULL);
    GtkWidget *txn_area = gtk_dialog_get_content_area(GTK_DIALOG(txn_dialog));
    GtkWidget *txn_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(txn_area), txn_box);

    // Customer info header
    char cust_info[NAME_LEN * 3];
    if (txn.customer_idx >= 0) {
        Customer *c = &s->customers[txn.customer_idx];
        sprintf(cust_info, "Customer: %s %s (%s)", c->first_name, c->last_name, c->company);
    } else {
        sprintf(cust_info, "Customer: Not selected");
    }
    gtk_box_pack_start(GTK_BOX(txn_box), gtk_label_new(cust_info), FALSE, FALSE, 0);

    // Keep open indicator (always on for layaways)
    GtkWidget *keep_open_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(keep_open_label), "<b>Layaway - Transaction will remain open</b>");
    gtk_box_pack_start(GTK_BOX(txn_box), keep_open_label, FALSE, FALSE, 0);

    // Item list display
    GtkWidget *item_sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(item_sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    GtkWidget *item_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(item_view), FALSE);
    gtk_container_add(GTK_CONTAINER(item_sw), item_view);
    gtk_box_pack_start(GTK_BOX(txn_box), item_sw, TRUE, TRUE, 0);

    GtkWidget *total_label = gtk_label_new("Total: $0.00");
    gtk_box_pack_start(GTK_BOX(txn_box), total_label, FALSE, FALSE, 0);

    gtk_widget_set_size_request(txn_dialog, 500, 400);
    gtk_widget_show_all(txn_dialog);

    // Transaction editing loop
    int done = 0;
    while (!done) {
        // Update item list display
        GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(item_view));
        char item_text[2000] = "LAYAWAY ITEMS:\n\n";
        for (int i = 0; i < txn.item_count; i++) {
            Product *p = NULL;
            for (int j = 0; j < s->product_count; j++) {
                if (strcmp(s->products[j].sku, txn.item_sku[i]) == 0) {
                    p = &s->products[j];
                    break;
                }
            }
            if (p) {
                char line[200];
                sprintf(line, "%d. %s (SKU: %s) Qty: %d @ $%.2f = $%.2f\n",
                        i + 1, p->name, p->sku, txn.qty[i], txn.item_price[i],
                        txn.item_price[i] * txn.qty[i]);
                strcat(item_text, line);
            }
        }
        gtk_text_buffer_set_text(buf, item_text, -1);

        char total_str[100];
        sprintf(total_str, "Total: $%.2f", txn.total);
        gtk_label_set_text(GTK_LABEL(total_label), total_str);

        int response = gtk_dialog_run(GTK_DIALOG(txn_dialog));

        if (response == 1) { // Add Item
            // Product search dialog
            GtkWidget *prod_dialog = gtk_dialog_new_with_buttons("Add Product to Layaway",
                                                                 GTK_WINDOW(main_window),
                                                                 GTK_DIALOG_MODAL,
                                                                 "_Add", GTK_RESPONSE_OK,
                                                                 "_Cancel", GTK_RESPONSE_CANCEL,
                                                                 NULL);
            GtkWidget *prod_area = gtk_dialog_get_content_area(GTK_DIALOG(prod_dialog));
            GtkWidget *prod_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
            gtk_container_add(GTK_CONTAINER(prod_area), prod_box);

            GtkWidget *sku_entry = create_labeled_entry("SKU/UPC:", prod_box);
            GtkWidget *qty_entry = create_labeled_entry("Quantity:", prod_box);
            gtk_entry_set_text(GTK_ENTRY(qty_entry), "1");

            gtk_widget_show_all(prod_dialog);

            if (gtk_dialog_run(GTK_DIALOG(prod_dialog)) == GTK_RESPONSE_OK) {
                const char *sku = gtk_entry_get_text(GTK_ENTRY(sku_entry));
                const char *qty_str = gtk_entry_get_text(GTK_ENTRY(qty_entry));

                Product *p = NULL;
                for (int j = 0; j < s->product_count; j++) {
                    if (strcmp(s->products[j].sku, sku) == 0) {
                        p = &s->products[j];
                        break;
                    }
                }

                if (p && txn.item_count < MAX_PRODUCTS) {
                    int qty = atoi(qty_str);
                    int is_special_order = 0;
                    char so_comments[NAME_LEN] = "";

                    // Check if out of stock
                    if (p->stock < qty) {
                        special_order_prompt_dialog(s, sku, qty, &is_special_order, so_comments);
                    }

                    strncpy(txn.item_sku[txn.item_count], sku, NAME_LEN - 1);
                    txn.item_price[txn.item_count] = p->price;
                    txn.qty[txn.item_count] = qty;
                    txn.is_special_order[txn.item_count] = is_special_order;
                    if (is_special_order) {
                        strncpy(txn.so_comments[txn.item_count], so_comments, NAME_LEN - 1);
                    }
                    txn.total += p->price * qty;
                    txn.item_count++;

                    // Create special order record if needed
                    if (is_special_order) {
                        SpecialOrder so;
                        memset(&so, 0, sizeof(SpecialOrder));
                        so.special_order_id = s->special_order_count;
                        strncpy(so.product_sku, sku, NAME_LEN-1);
                        so.product_sku[NAME_LEN-1] = '\0';
                        so.store_idx = si;
                        so.customer_idx = txn.customer_idx;
                        so.qty_ordered = qty;
                        so.status = SO_STATUS_NOT_ORDERED;
                        so.type = (strcmp(so_comments, "Transfer requested from another store") == 0) ? SO_TYPE_REQUEST_TRANSFER : SO_TYPE_MARK_FOR_SO;
                        strncpy(so.comments, so_comments, sizeof(so.comments)-1);
                        so.comments[sizeof(so.comments)-1] = '\0';
                        time_t now = time(NULL);
                        struct tm *tm_info = localtime(&now);
                        strftime(so.order_date, NAME_LEN, "%Y-%m-%d", tm_info);
                        strcpy(so.expected_date, "");
                        strcpy(so.received_date, "");
                        strcpy(so.po_number, "");
                        so.transfer_store_idx = -1;
                        strcpy(so.serial_number, "");
                        so.notified = 0;
                        strcpy(so.notification_method, "");

                        s->special_orders[s->special_order_count++] = so;
                    }
                } else if (!p) {
                    show_error_dialog("Product not found!");
                } else {
                    show_error_dialog("Maximum items reached for this transaction.");
                }
            }

            gtk_widget_destroy(prod_dialog);

        } else if (response == 2) { // Take Payment
            // Payment dialog
            GtkWidget *pay_dialog = gtk_dialog_new_with_buttons("Take Payment for Layaway",
                                                                GTK_WINDOW(main_window),
                                                                GTK_DIALOG_MODAL,
                                                                "_Apply Payment", 1,
                                                                "_Cancel", GTK_RESPONSE_CANCEL,
                                                                NULL);
            GtkWidget *pay_area = gtk_dialog_get_content_area(GTK_DIALOG(pay_dialog));
            GtkWidget *pay_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
            gtk_container_add(GTK_CONTAINER(pay_area), pay_box);

            char pay_info[200];
            sprintf(pay_info, "Total: $%.2f\nPaid: $%.2f\nRemaining: $%.2f",
                    txn.total, txn.amount_paid, txn.total - txn.amount_paid);
            gtk_box_pack_start(GTK_BOX(pay_box), gtk_label_new(pay_info), FALSE, FALSE, 0);

            GtkWidget *amount_entry = create_labeled_entry("Payment Amount:", pay_box);
            char default_amount[20];
            sprintf(default_amount, "%.2f", txn.total - txn.amount_paid);
            gtk_entry_set_text(GTK_ENTRY(amount_entry), default_amount);

            GtkWidget *payment_combo = gtk_combo_box_text_new();
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(payment_combo), "Cash");
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(payment_combo), "Credit");
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(payment_combo), "Debit");
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(payment_combo), "Gift Card");
            gtk_combo_box_set_active(GTK_COMBO_BOX(payment_combo), 0);
            gtk_box_pack_start(GTK_BOX(pay_box), gtk_label_new("Payment Type:"), FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(pay_box), payment_combo, FALSE, FALSE, 0);

            gtk_widget_show_all(pay_dialog);

            if (gtk_dialog_run(GTK_DIALOG(pay_dialog)) == 1) {
                const char *amount_str = gtk_entry_get_text(GTK_ENTRY(amount_entry));
                double payment = atof(amount_str);
                int payment_type = gtk_combo_box_get_active(GTK_COMBO_BOX(payment_combo));

                if (payment > 0) {
                    txn.amount_paid += payment;
                    txn.payment_type = payment_type;

                    // Update product stock
                    for (int i = 0; i < txn.item_count; i++) {
                        Product *p = NULL;
                        for (int j = 0; j < s->product_count; j++) {
                            if (strcmp(s->products[j].sku, txn.item_sku[i]) == 0) {
                                p = &s->products[j];
                                break;
                            }
                        }
                        if (p) {
                            p->sold += txn.qty[i];
                            // Don't reduce stock for layaways - items stay in inventory
                        }
                    }

                    // Set transaction date
                    time_t now = time(NULL);
                    struct tm *tm_info = localtime(&now);
                    strftime(txn.date, NAME_LEN, "%Y-%m-%d %H:%M", tm_info);

                    // Save transaction
                    s->sales[s->sales_count] = txn;
                    s->sales_count++;
                    s->transactions++;

                    char msg[150];
                    double remaining = txn.total - txn.amount_paid;
                    sprintf(msg, "Layaway Payment Applied\n\nTotal: $%.2f\nPaid: $%.2f\nRemaining: $%.2f\n\nTransaction will remain open.",
                            txn.total, txn.amount_paid, remaining);
                    show_info_dialog(msg);

                    done = 1;
                }
            }

            gtk_widget_destroy(pay_dialog);

        } else if (response == 3) { // Save Layaway
            if (txn.item_count == 0) {
                show_error_dialog("Cannot save layaway with no items!");
                continue;
            }

            // Set transaction date
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);
            strftime(txn.date, NAME_LEN, "%Y-%m-%d %H:%M", tm_info);

            // Save transaction
            s->sales[s->sales_count] = txn;
            s->sales_count++;
            s->transactions++;

            char msg[150];
            sprintf(msg, "Layaway Created\n\nTotal: $%.2f\nPaid: $%.2f\nRemaining: $%.2f\n\nTransaction saved as open layaway.",
                    txn.total, txn.amount_paid, txn.total - txn.amount_paid);
            show_info_dialog(msg);

            done = 1;

        } else { // Cancel
            done = 1;
        }
    }

    gtk_widget_destroy(txn_dialog);
}

static void on_return_item_toggle(GtkCellRendererToggle *renderer, gchar *path_str, gpointer data) {
        (void)renderer;
        GtkListStore *store = GTK_LIST_STORE(data);
        GtkTreeIter iter;
        GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
        gboolean active;
        gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path);
        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &active, -1);
        gtk_list_store_set(store, &iter, 0, !active, -1);
        gtk_tree_path_free(path);
    }

static void create_return_dialog(void) {
        int si = choose_store_index();
        if (si < 0) return;
        Store *s = &stores[si];

        if (s->sales_count >= MAX_TRANSACTIONS) {
            show_error_dialog("Maximum transactions reached for this store.");
            return;
        }

        // Initialize return transaction
        Transaction ret_txn;
        memset(&ret_txn, 0, sizeof(Transaction));
        ret_txn.status = 1; // completed
        ret_txn.customer_idx = -1;
        ret_txn.is_return = 1;
        sprintf(ret_txn.transaction_id, "RET-%d-%ld", si, (long)time(NULL) % 10000);
        strcpy(ret_txn.original_transaction_id, "");

        // === STEP 1: Previous Sales Items window ===
        GtkWidget *prev_dialog = gtk_dialog_new_with_buttons("Previous Sales Items",
                                                             GTK_WINDOW(main_window),
                                                             GTK_DIALOG_MODAL,
                                                             "_Lookup", 1,
                                                             "_Add", 2,
                                                             "_No Receipt", GTK_RESPONSE_CANCEL,
                                                             NULL);
        GtkWidget *prev_area = gtk_dialog_get_content_area(GTK_DIALOG(prev_dialog));
        GtkWidget *prev_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_set_border_width(GTK_CONTAINER(prev_vbox), 10);
        gtk_container_add(GTK_CONTAINER(prev_area), prev_vbox);

        gtk_box_pack_start(GTK_BOX(prev_vbox),
            gtk_label_new("Scan or type the barcode from the customer's receipt, then click Lookup.\n"
                          "Click 'No Receipt' to add return items manually."),
            FALSE, FALSE, 0);

        GtkWidget *barcode_entry = create_labeled_entry("Sales Receipt Number:", prev_vbox);

        GtkWidget *items_sw = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(items_sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_widget_set_size_request(items_sw, -1, 180);
        gtk_box_pack_start(GTK_BOX(prev_vbox), items_sw, TRUE, TRUE, 0);

        // columns: 0=return(bool), 1=product name, 2=sku, 3=qty(int), 4=price(double), 5=item_idx(int)
        GtkListStore *items_store = gtk_list_store_new(6,
                                                        G_TYPE_BOOLEAN,
                                                        G_TYPE_STRING,
                                                        G_TYPE_STRING,
                                                        G_TYPE_INT,
                                                        G_TYPE_DOUBLE,
                                                        G_TYPE_INT);

        GtkTreeView *items_tree = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(items_store)));

        GtkCellRenderer *toggle_rend = gtk_cell_renderer_toggle_new();
        gtk_cell_renderer_toggle_set_activatable(GTK_CELL_RENDERER_TOGGLE(toggle_rend), TRUE);
        g_signal_connect(toggle_rend, "toggled", G_CALLBACK(on_return_item_toggle), items_store);
        gtk_tree_view_insert_column_with_attributes(items_tree, -1, "Return", toggle_rend, "active", 0, NULL);

        GtkCellRenderer *text_rend = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes(items_tree, -1, "Product", text_rend, "text", 1, NULL);
        gtk_tree_view_insert_column_with_attributes(items_tree, -1, "SKU", text_rend, "text", 2, NULL);
        gtk_tree_view_insert_column_with_attributes(items_tree, -1, "Qty", text_rend, "text", 3, NULL);
        gtk_tree_view_insert_column_with_attributes(items_tree, -1, "Price", text_rend, "text", 4, NULL);

        gtk_container_add(GTK_CONTAINER(items_sw), GTK_WIDGET(items_tree));
        gtk_widget_set_size_request(prev_dialog, 580, 440);
        gtk_widget_show_all(prev_dialog);

        Transaction *orig_txn = NULL;
        int from_receipt = 0;
        int done_prev = 0;
        while (!done_prev) {
            int resp = gtk_dialog_run(GTK_DIALOG(prev_dialog));
            if (resp == 1) { // Lookup
                const char *barcode = gtk_entry_get_text(GTK_ENTRY(barcode_entry));
                if (strlen(barcode) == 0) {
                    show_error_dialog("Please enter a receipt number.");
                    continue;
                }
                orig_txn = NULL;
                for (int i = 0; i < s->sales_count; i++) {
                    if (!s->sales[i].is_return && strcmp(s->sales[i].transaction_id, barcode) == 0) {
                        orig_txn = &s->sales[i];
                        break;
                    }
                }
                if (!orig_txn) {
                    show_error_dialog("Transaction not found. Check the receipt number.");
                    continue;
                }
                // Populate items list (all pre-checked)
                gtk_list_store_clear(items_store);
                for (int k = 0; k < orig_txn->item_count; k++) {
                    if (orig_txn->qty[k] < 0) continue; // skip already-return items
                    Product *p = NULL;
                    for (int j = 0; j < s->product_count; j++) {
                        if (strcmp(s->products[j].sku, orig_txn->item_sku[k]) == 0) {
                            p = &s->products[j];
                            break;
                        }
                    }
                    GtkTreeIter iter;
                    gtk_list_store_append(items_store, &iter);
                    gtk_list_store_set(items_store, &iter,
                                      0, TRUE,
                                      1, p ? p->name : orig_txn->item_sku[k],
                                      2, orig_txn->item_sku[k],
                                      3, orig_txn->qty[k],
                                      4, orig_txn->item_price[k],
                                      5, k,
                                      -1);
                }
            } else if (resp == 2) { // Add
                if (!orig_txn) {
                    show_error_dialog("Please click Lookup to find a transaction first,\nor click 'No Receipt' to add items manually.");
                    continue;
                }
                // Verify at least one item is checked
                gboolean any_sel = FALSE;
                GtkTreeIter iter;
                gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(items_store), &iter);
                while (valid) {
                    gboolean sel;
                    gtk_tree_model_get(GTK_TREE_MODEL(items_store), &iter, 0, &sel, -1);
                    if (sel) { any_sel = TRUE; break; }
                    valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(items_store), &iter);
                }
                if (!any_sel) {
                    show_error_dialog("Please check at least one item to return.");
                    continue;
                }
                // Build return items from checked rows
                valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(items_store), &iter);
                while (valid && ret_txn.item_count < MAX_PRODUCTS) {
                    gboolean sel;
                    gchar *sku_str;
                    gint qty_val;
                    gdouble price_val;
                    gtk_tree_model_get(GTK_TREE_MODEL(items_store), &iter,
                                       0, &sel, 2, &sku_str, 3, &qty_val, 4, &price_val, -1);
                    if (sel) {
                        strncpy(ret_txn.item_sku[ret_txn.item_count], sku_str, NAME_LEN - 1);
                        ret_txn.item_price[ret_txn.item_count] = price_val;
                        ret_txn.qty[ret_txn.item_count] = -qty_val; // NEGATIVE qty = return
                        ret_txn.total += price_val * (-qty_val);
                        ret_txn.item_count++;
                    }
                    g_free(sku_str);
                    valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(items_store), &iter);
                }
                strncpy(ret_txn.original_transaction_id, orig_txn->transaction_id, NAME_LEN - 1);
                from_receipt = 1;
                done_prev = 1;
            } else { // No Receipt or window closed
                from_receipt = 0;
                orig_txn = NULL;
                done_prev = 1;
            }
        }
        gtk_widget_destroy(prev_dialog);

        // === STEP 2: Ask about crediting original customer (receipt mode) ===
        if (from_receipt && orig_txn) {
            int orig_ci = orig_txn->customer_idx;
            if (orig_ci >= 0 && orig_ci < s->customer_count) {
                Customer *oc = &s->customers[orig_ci];
                char cq_msg[256];
                sprintf(cq_msg,
                        "Would you like to credit the selected return items\n"
                        "to the original customer?\n\nOriginal Customer: %s %s",
                        oc->first_name, oc->last_name);
                GtkWidget *cq_dlg = gtk_dialog_new_with_buttons("Credit Original Customer?",
                                                                GTK_WINDOW(main_window),
                                                                GTK_DIALOG_MODAL,
                                                                "_Yes", GTK_RESPONSE_YES,
                                                                "_No", GTK_RESPONSE_NO,
                                                                NULL);
                GtkWidget *cq_area = gtk_dialog_get_content_area(GTK_DIALOG(cq_dlg));
                gtk_container_set_border_width(GTK_CONTAINER(cq_area), 15);
                GtkWidget *cq_label = gtk_label_new(cq_msg);
                gtk_container_add(GTK_CONTAINER(cq_area), cq_label);
                gtk_widget_show_all(cq_dlg);
                if (gtk_dialog_run(GTK_DIALOG(cq_dlg)) == GTK_RESPONSE_YES) {
                    ret_txn.customer_idx = orig_ci;
                }
                gtk_widget_destroy(cq_dlg);
            }
        }

        // === STEP 3: Customer selection if not yet attached ===
        if (ret_txn.customer_idx < 0) {
            if (s->customer_count == 0) {
                show_error_dialog("No customers in this store. Add a customer first.");
                return;
            }
            GtkWidget *cust_dlg = gtk_dialog_new_with_buttons("Select Customer for Return",
                                                             GTK_WINDOW(main_window),
                                                             GTK_DIALOG_MODAL,
                                                             "_OK", GTK_RESPONSE_OK,
                                                             "_Cancel", GTK_RESPONSE_CANCEL,
                                                             NULL);
            GtkWidget *cust_area = gtk_dialog_get_content_area(GTK_DIALOG(cust_dlg));
            GtkWidget *cust_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
            gtk_container_set_border_width(GTK_CONTAINER(cust_box), 10);
            gtk_container_add(GTK_CONTAINER(cust_area), cust_box);
            GtkWidget *cust_combo = gtk_combo_box_text_new();
            for (int i = 0; i < s->customer_count; i++) {
                if (s->customers[i].hidden) continue;
                Customer *c = &s->customers[i];
                char entry[NAME_LEN * 2];
                sprintf(entry, "%s %s (%s)", c->first_name, c->last_name, c->company);
                gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cust_combo), entry);
            }
            gtk_box_pack_start(GTK_BOX(cust_box), gtk_label_new("Select Customer:"), FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(cust_box), cust_combo, FALSE, FALSE, 0);
            gtk_widget_show_all(cust_dlg);
            if (gtk_dialog_run(GTK_DIALOG(cust_dlg)) != GTK_RESPONSE_OK) {
                gtk_widget_destroy(cust_dlg);
                return;
            }
            int ci = gtk_combo_box_get_active(GTK_COMBO_BOX(cust_combo));
            if (ci >= 0 && ci < s->customer_count) {
                ret_txn.customer_idx = ci;
            }
            gtk_widget_destroy(cust_dlg);
        }

        // === STEP 4: Return transaction editor ===
        GtkWidget *ret_edit = gtk_dialog_new_with_buttons("Return Transaction",
                                                          GTK_WINDOW(main_window),
                                                          GTK_DIALOG_MODAL,
                                                          "_Add Item", 1,
                                                          "_Process Refund", 2,
                                                          "_Cancel", GTK_RESPONSE_CANCEL,
                                                          NULL);
        GtkWidget *ret_area = gtk_dialog_get_content_area(GTK_DIALOG(ret_edit));
        GtkWidget *ret_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_set_border_width(GTK_CONTAINER(ret_box), 8);
        gtk_container_add(GTK_CONTAINER(ret_area), ret_box);

        // Customer header
        char cust_hdr[NAME_LEN * 3];
        if (ret_txn.customer_idx >= 0) {
            Customer *c = &s->customers[ret_txn.customer_idx];
            sprintf(cust_hdr, "Customer: %s %s", c->first_name, c->last_name);
        } else {
            sprintf(cust_hdr, "Customer: Not selected");
        }
        gtk_box_pack_start(GTK_BOX(ret_box), gtk_label_new(cust_hdr), FALSE, FALSE, 0);

        if (strlen(ret_txn.original_transaction_id) > 0) {
            char orig_hdr[NAME_LEN + 32];
            sprintf(orig_hdr, "Original Transaction: %s", ret_txn.original_transaction_id);
            gtk_box_pack_start(GTK_BOX(ret_box), gtk_label_new(orig_hdr), FALSE, FALSE, 0);
        }

        GtkWidget *ret_note = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(ret_note), "<b>RETURN - negative quantities indicate items being refunded</b>");
        gtk_box_pack_start(GTK_BOX(ret_box), ret_note, FALSE, FALSE, 0);

        GtkWidget *item_sw2 = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(item_sw2), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        GtkWidget *item_view2 = gtk_text_view_new();
        gtk_text_view_set_editable(GTK_TEXT_VIEW(item_view2), FALSE);
        gtk_container_add(GTK_CONTAINER(item_sw2), item_view2);
        gtk_box_pack_start(GTK_BOX(ret_box), item_sw2, TRUE, TRUE, 0);

        GtkWidget *balance_label = gtk_label_new("Balance: $0.00");
        gtk_box_pack_start(GTK_BOX(ret_box), balance_label, FALSE, FALSE, 0);

        gtk_widget_set_size_request(ret_edit, 520, 420);
        gtk_widget_show_all(ret_edit);

        int done_ret = 0;
        while (!done_ret) {
            // Refresh item list
            GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(item_view2));
            char item_text[3000] = "RETURN ITEMS:\n\n";
            for (int i = 0; i < ret_txn.item_count; i++) {
                Product *p = NULL;
                for (int j = 0; j < s->product_count; j++) {
                    if (strcmp(s->products[j].sku, ret_txn.item_sku[i]) == 0) {
                        p = &s->products[j];
                        break;
                    }
                }
                char line[256];
                sprintf(line, "%d. %s (SKU: %s)  Qty: %d  @ $%.2f = $%.2f\n",
                        i + 1,
                        p ? p->name : ret_txn.item_sku[i],
                        ret_txn.item_sku[i],
                        ret_txn.qty[i],
                        ret_txn.item_price[i],
                        ret_txn.item_price[i] * ret_txn.qty[i]);
                strncat(item_text, line, sizeof(item_text) - strlen(item_text) - 1);
            }
            gtk_text_buffer_set_text(buf, item_text, -1);

            // Balance in parens if negative (store pays customer)
            char bal_str[64];
            if (ret_txn.total < 0.0) {
                sprintf(bal_str, "Balance: ($%.2f)", -ret_txn.total);
            } else {
                sprintf(bal_str, "Balance: $%.2f", ret_txn.total);
            }
            gtk_label_set_text(GTK_LABEL(balance_label), bal_str);

            int resp2 = gtk_dialog_run(GTK_DIALOG(ret_edit));

            if (resp2 == 1) { // Add Item manually
                GtkWidget *add_dlg = gtk_dialog_new_with_buttons("Add Return Item",
                                                                 GTK_WINDOW(main_window),
                                                                 GTK_DIALOG_MODAL,
                                                                 "_Add", GTK_RESPONSE_OK,
                                                                 "_Cancel", GTK_RESPONSE_CANCEL,
                                                                 NULL);
                GtkWidget *add_area = gtk_dialog_get_content_area(GTK_DIALOG(add_dlg));
                GtkWidget *add_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
                gtk_container_set_border_width(GTK_CONTAINER(add_box), 10);
                gtk_container_add(GTK_CONTAINER(add_area), add_box);

                GtkWidget *sku_e = create_labeled_entry("SKU/UPC:", add_box);
                GtkWidget *qty_e = create_labeled_entry("Quantity (positive, will be returned):", add_box);
                gtk_entry_set_text(GTK_ENTRY(qty_e), "1");
                GtkWidget *price_e = create_labeled_entry("Refund Price:", add_box);

                gtk_box_pack_start(GTK_BOX(add_box),
                    gtk_label_new("Note: Ascend uses the lowest price the item was\never sold for to prevent fraud. Adjust if needed."),
                    FALSE, FALSE, 0);

                gtk_widget_show_all(add_dlg);

                if (gtk_dialog_run(GTK_DIALOG(add_dlg)) == GTK_RESPONSE_OK) {
                    const char *sku_in = gtk_entry_get_text(GTK_ENTRY(sku_e));
                    int qty_in = atoi(gtk_entry_get_text(GTK_ENTRY(qty_e)));
                    const char *price_in_str = gtk_entry_get_text(GTK_ENTRY(price_e));

                    Product *p = NULL;
                    for (int j = 0; j < s->product_count; j++) {
                        if (strcmp(s->products[j].sku, sku_in) == 0) {
                            p = &s->products[j];
                            break;
                        }
                    }

                    if (!p) {
                        show_error_dialog("Product not found. Check SKU/UPC.");
                    } else if (qty_in <= 0) {
                        show_error_dialog("Quantity must be a positive number.");
                    } else if (ret_txn.item_count < MAX_PRODUCTS) {
                        // Find lowest price this item was ever sold for
                        double lowest = p->price;
                        for (int t = 0; t < s->sales_count; t++) {
                            if (s->sales[t].is_return) continue;
                            for (int k = 0; k < s->sales[t].item_count; k++) {
                                if (strcmp(s->sales[t].item_sku[k], sku_in) == 0
                                        && s->sales[t].item_price[k] > 0.0
                                        && s->sales[t].item_price[k] < lowest) {
                                    lowest = s->sales[t].item_price[k];
                                }
                            }
                        }

                        // Use entered price if provided, otherwise lowest
                        double refund_price = (strlen(price_in_str) > 0 && atof(price_in_str) > 0)
                                             ? atof(price_in_str) : lowest;

                        strncpy(ret_txn.item_sku[ret_txn.item_count], sku_in, NAME_LEN - 1);
                        ret_txn.item_price[ret_txn.item_count] = refund_price;
                        ret_txn.qty[ret_txn.item_count] = -qty_in;
                        ret_txn.total += refund_price * (-qty_in);
                        ret_txn.item_count++;
                    } else {
                        show_error_dialog("Maximum items reached for this transaction.");
                    }
                }
                gtk_widget_destroy(add_dlg);

            } else if (resp2 == 2) { // Process Refund
                if (ret_txn.item_count == 0) {
                    show_error_dialog("No items to return. Add items first.");
                    continue;
                }
                done_ret = 1;
            } else { // Cancel
                gtk_widget_destroy(ret_edit);
                return;
            }
        }
        gtk_widget_destroy(ret_edit);

        // === STEP 5: Refund payment dialog ===
        GtkWidget *refund_dlg = gtk_dialog_new_with_buttons("Process Refund",
                                                            GTK_WINDOW(main_window),
                                                            GTK_DIALOG_MODAL,
                                                            "_Complete Return", 1,
                                                            "_Cancel", GTK_RESPONSE_CANCEL,
                                                            NULL);
        GtkWidget *refund_area = gtk_dialog_get_content_area(GTK_DIALOG(refund_dlg));
        GtkWidget *refund_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_set_border_width(GTK_CONTAINER(refund_box), 10);
        gtk_container_add(GTK_CONTAINER(refund_area), refund_box);

        char refund_total_str[64];
        sprintf(refund_total_str, "Refund Amount: ($%.2f)", -ret_txn.total);
        gtk_box_pack_start(GTK_BOX(refund_box), gtk_label_new(refund_total_str), FALSE, FALSE, 0);

        GtkWidget *refund_type_combo = gtk_combo_box_text_new();
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(refund_type_combo), "Cash");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(refund_type_combo), "Credit Card");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(refund_type_combo), "In-Store Credit (Gift Card)");
        gtk_combo_box_set_active(GTK_COMBO_BOX(refund_type_combo), 0);
        gtk_box_pack_start(GTK_BOX(refund_box), gtk_label_new("Refund Method:"), FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(refund_box), refund_type_combo, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(refund_box),
            gtk_label_new("Note: Debit payments must be refunded as Credit.\n"
                          "Card issuers may take up to 14 business days to process."),
            FALSE, FALSE, 5);

        gtk_widget_show_all(refund_dlg);

        if (gtk_dialog_run(GTK_DIALOG(refund_dlg)) == 1) { // Complete Return
            int ptype = gtk_combo_box_get_active(GTK_COMBO_BOX(refund_type_combo));
            ret_txn.payment_type = (ptype == 2) ? PAYMENT_GIFT : (ptype == 1 ? PAYMENT_CREDIT : PAYMENT_CASH);
            ret_txn.amount_paid = ret_txn.total; // refund = total (negative)

            // Set date
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);
            strftime(ret_txn.date, NAME_LEN, "%Y-%m-%d %H:%M", tm_info);

            // Re-add returned items to inventory
            for (int i = 0; i < ret_txn.item_count; i++) {
                for (int j = 0; j < s->product_count; j++) {
                    if (strcmp(s->products[j].sku, ret_txn.item_sku[i]) == 0) {
                        s->products[j].stock += (-ret_txn.qty[i]); // qty is negative; negate it
                        break;
                    }
                }
            }

            // Deduct returned amount from sales_to_date
            s->sales_to_date += ret_txn.total; // total is negative, so this subtracts

            // Save return transaction
            s->sales[s->sales_count++] = ret_txn;
            s->transactions++;

            char done_msg[256];
            sprintf(done_msg, "Return Completed!\n\nTransaction ID: %s\nRefund: ($%.2f)\n\nItems have been re-added to inventory.",
                    ret_txn.transaction_id, -ret_txn.total);
            show_info_dialog(done_msg);
            save_data();
        }

        gtk_widget_destroy(refund_dlg);
    }

static void complete_sale_dialog(void) {

    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Complete-a-Sale Utility",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Find Transaction", 1,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *search_entry = create_labeled_entry("Transaction ID or Customer Name:", vbox);

    gtk_widget_show_all(dialog);

    int done = 0;
    while (!done) {
        int response = gtk_dialog_run(GTK_DIALOG(dialog));

        if (response == 1) { // Find Transaction
            const char *search_term = gtk_entry_get_text(GTK_ENTRY(search_entry));

            if (strlen(search_term) == 0) {
                show_error_dialog("Please enter a transaction ID or customer name.");
                continue;
            }

            // Search for matching transactions
            Transaction *found_txn = NULL;
            int found_idx = -1;

            for (int i = 0; i < s->sales_count; i++) {
                Transaction *txn = &s->sales[i];
                if (txn->status == 0) { // only open transactions
                    // Check transaction ID
                    if (strstr(txn->transaction_id, search_term) != NULL) {
                        found_txn = txn;
                        found_idx = i;
                        break;
                    }

                    // Check customer name
                    if (txn->customer_idx >= 0 && txn->customer_idx < s->customer_count) {
                        Customer *c = &s->customers[txn->customer_idx];
                        char full_name[NAME_LEN * 2];
                        sprintf(full_name, "%s %s", c->first_name, c->last_name);
                        if (strstr(full_name, search_term) != NULL) {
                            found_txn = txn;
                            found_idx = i;
                            break;
                        }
                    }
                }
            }

            if (found_txn) {
                // Show transaction details and allow completion
                char txn_info[1000] = "TRANSACTION FOUND:\n\n";
                char line[200];

                sprintf(line, "Transaction ID: %s\n", found_txn->transaction_id);
                strcat(txn_info, line);

                if (found_txn->customer_idx >= 0 && found_txn->customer_idx < s->customer_count) {
                    Customer *c = &s->customers[found_txn->customer_idx];
                    sprintf(line, "Customer: %s %s\n", c->first_name, c->last_name);
                    strcat(txn_info, line);
                }

                sprintf(line, "Total: $%.2f\nPaid: $%.2f\nRemaining: $%.2f\n\n",
                        found_txn->total, found_txn->amount_paid, found_txn->total - found_txn->amount_paid);
                strcat(txn_info, line);

                strcat(txn_info, "Items:\n");
                for (int i = 0; i < found_txn->item_count; i++) {
                    Product *p = NULL;
                    for (int j = 0; j < s->product_count; j++) {
                        if (strcmp(s->products[j].sku, found_txn->item_sku[i]) == 0) {
                            p = &s->products[j];
                            break;
                        }
                    }
                    sprintf(line, "  %s (Qty: %d) - $%.2f\n",
                            p ? p->name : found_txn->item_sku[i], found_txn->qty[i], found_txn->item_price[i]);
                    strcat(txn_info, line);
                }

                GtkWidget *detail_dialog = gtk_dialog_new_with_buttons("Transaction Details",
                                                                     GTK_WINDOW(main_window),
                                                                     GTK_DIALOG_MODAL,
                                                                     "_Complete as Sale", 1,
                                                                     "_Cancel", GTK_RESPONSE_CANCEL,
                                                                     NULL);
                GtkWidget *detail_area = gtk_dialog_get_content_area(GTK_DIALOG(detail_dialog));
                GtkWidget *detail_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
                gtk_container_set_border_width(GTK_CONTAINER(detail_vbox), 10);
                gtk_container_add(GTK_CONTAINER(detail_area), detail_vbox);

                GtkWidget *info_label = gtk_label_new(txn_info);
                gtk_box_pack_start(GTK_BOX(detail_vbox), info_label, FALSE, FALSE, 0);

                gtk_widget_show_all(detail_dialog);

                if (gtk_dialog_run(GTK_DIALOG(detail_dialog)) == 1) {
                    // Complete the transaction
                    found_txn->status = 1; // completed
                    s->sales_to_date += found_txn->total;

                    // Update product stock (reduce inventory since items are now sold)
                    for (int i = 0; i < found_txn->item_count; i++) {
                        Product *p = NULL;
                        for (int j = 0; j < s->product_count; j++) {
                            if (strcmp(s->products[j].sku, found_txn->item_sku[i]) == 0) {
                                p = &s->products[j];
                                break;
                            }
                        }
                        if (p) {
                            p->stock -= found_txn->qty[i];
                            p->sold += found_txn->qty[i];
                        }
                    }

                    show_info_dialog("Transaction completed successfully!");
                    save_data();
                    done = 1;
                }

                gtk_widget_destroy(detail_dialog);

            } else {
                show_error_dialog("No matching open transaction found.");
            }

        } else { // Cancel
            done = 1;
        }
    }

    gtk_widget_destroy(dialog);
}

static void special_order_prompt_dialog(Store *s, const char *sku, int qty, int *is_special_order, char *comments) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Special Order Options",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Associate to PO", 1,
                                                   "_Mark for Order", 2,
                                                   "_No Special Order", 3,
                                                   "_Request Transfer", 4,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    // Product info
    Product *p = NULL;
    for (int i = 0; i < s->product_count; i++) {
        if (strcmp(s->products[i].sku, sku) == 0) {
            p = &s->products[i];
            break;
        }
    }

    char info[200];
    sprintf(info, "Product: %s\nSKU: %s\nRequested Qty: %d\nAvailable Stock: %d",
            p ? p->name : "Unknown", sku, qty, p ? p->stock : 0);
    GtkWidget *info_label = gtk_label_new(info);
    gtk_box_pack_start(GTK_BOX(vbox), info_label, FALSE, FALSE, 0);

    // Comments field
    GtkWidget *comments_entry = create_labeled_entry("Comments (color, size, etc.):", vbox);

    gtk_widget_show_all(dialog);

    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    *is_special_order = 0;

    if (response == 1) { // Associate to Purchase Order
        // For now, just mark as special order - full PO integration would need more complex logic
        *is_special_order = 1;
        const char *comm = gtk_entry_get_text(GTK_ENTRY(comments_entry));
        if (comm && strlen(comm) > 0) {
            strncpy(comments, comm, NAME_LEN - 1);
        } else {
            strcpy(comments, "Associated to Purchase Order");
        }
    } else if (response == 2) { // Mark for Special Order
        *is_special_order = 1;
        const char *comm = gtk_entry_get_text(GTK_ENTRY(comments_entry));
        if (comm && strlen(comm) > 0) {
            strncpy(comments, comm, NAME_LEN - 1);
        } else {
            strcpy(comments, "Marked for special order");
        }
    } else if (response == 3) { // No Special Order
        *is_special_order = 0;
        // Note: User should manually adjust inventory later
    } else if (response == 4) { // Request Transfer
        *is_special_order = 1;
        const char *comm = gtk_entry_get_text(GTK_ENTRY(comments_entry));
        if (comm && strlen(comm) > 0) {
            strncpy(comments, comm, NAME_LEN - 1);
        } else {
            strcpy(comments, "Transfer requested from another store");
        }
    }

    gtk_widget_destroy(dialog);
}

static void reorder_list_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Reorder List - Items to Order",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Order Selected", 1,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

    GtkListStore *store_list = gtk_list_store_new(8,
                                                   G_TYPE_BOOLEAN, // Checkbox
                                                   G_TYPE_STRING,  // SKU
                                                   G_TYPE_STRING,  // Product
                                                   G_TYPE_STRING,  // Customer
                                                   G_TYPE_INT,     // Qty
                                                   G_TYPE_STRING,  // Type
                                                   G_TYPE_STRING,  // Comments
                                                   G_TYPE_INT);    // SO Index

    GtkTreeView *tree = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_list)));

    // Checkbox column
    GtkCellRenderer *toggle_renderer = gtk_cell_renderer_toggle_new();
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Order", toggle_renderer, "active", 0, NULL);

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(tree, -1, "SKU", renderer, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Product", renderer, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Customer", renderer, "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Qty", renderer, "text", 4, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Type", renderer, "text", 5, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Comments", renderer, "text", 6, NULL);

    // Populate list with NOT_YET_ORDERED special orders
    for (int i = 0; i < s->special_order_count; i++) {
        SpecialOrder *so = &s->special_orders[i];
        if (so->status != SO_STATUS_NOT_ORDERED) continue;

        Product *p = NULL;
        for (int j = 0; j < s->product_count; j++) {
            if (strcmp(s->products[j].sku, so->product_sku) == 0) {
                p = &s->products[j];
                break;
            }
        }

        char customer_name[NAME_LEN * 2] = "Unknown";
        if (so->customer_idx >= 0 && so->customer_idx < s->customer_count) {
            Customer *c = &s->customers[so->customer_idx];
            sprintf(customer_name, "%s %s", c->first_name, c->last_name);
        }

        const char *status_names[] = {"On Order", "Received", "Completed", "Cancelled"};
        char status_str[32];
        if (so->status >= 0 && so->status <= 3) {
            strcpy(status_str, status_names[so->status]);
        } else {
            strcpy(status_str, "Unknown");
        }

        GtkTreeIter iter;
        gtk_list_store_append(store_list, &iter);
        gtk_list_store_set(store_list, &iter,
                          0, FALSE,  // Checkbox unchecked by default
                          1, so->product_sku,
                          2, p ? p->name : so->product_sku,
                          3, customer_name,
                          4, so->qty_ordered,
                          5, status_str,
                          6, so->comments,
                          7, i,
                          -1);
    }

    gtk_container_add(GTK_CONTAINER(sw), GTK_WIDGET(tree));

    // Store the list store and tree for the callback
    g_object_set_data(G_OBJECT(dialog), "store_list", store_list);
    g_object_set_data(G_OBJECT(dialog), "tree_view", tree);
    g_object_set_data(G_OBJECT(dialog), "store", s);

    gtk_widget_show_all(dialog);
    int result = gtk_dialog_run(GTK_DIALOG(dialog));

    if (result == 1) {  // Order Selected
        // Process selected items
        GtkTreeIter iter;
        gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store_list), &iter);
        int ordered_count = 0;

        while (valid) {
            gboolean selected;
            int so_index;
            gtk_tree_model_get(GTK_TREE_MODEL(store_list), &iter, 0, &selected, 7, &so_index, -1);

            if (selected) {
                SpecialOrder *so = &s->special_orders[so_index];
                so->status = SO_STATUS_ON_ORDER;
                ordered_count++;
            }

            valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store_list), &iter);
        }

        if (ordered_count > 0) {
            char msg[128];
            sprintf(msg, "%d item(s) marked as ordered.", ordered_count);
            show_info_dialog(msg);
            save_data();
        }
    }

    gtk_widget_destroy(dialog);
}

static void special_orders_on_order_report(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Special Order Items On Order",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Close", GTK_RESPONSE_CLOSE,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

    GtkListStore *store_list = gtk_list_store_new(6,
                                                   G_TYPE_STRING,  // SKU
                                                   G_TYPE_STRING,  // Product
                                                   G_TYPE_STRING,  // Customer
                                                   G_TYPE_INT,     // Qty
                                                   G_TYPE_STRING,  // Order Date
                                                   G_TYPE_STRING); // Comments

    GtkTreeView *tree = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_list)));

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(tree, -1, "SKU", renderer, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Product", renderer, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Customer", renderer, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Qty", renderer, "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Order Date", renderer, "text", 4, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Comments", renderer, "text", 5, NULL);

    // Populate list with ON_ORDER special orders
    for (int i = 0; i < s->special_order_count; i++) {
        SpecialOrder *so = &s->special_orders[i];
        if (so->status != SO_STATUS_ON_ORDER) continue;

        Product *p = NULL;
        for (int j = 0; j < s->product_count; j++) {
            if (strcmp(s->products[j].sku, so->product_sku) == 0) {
                p = &s->products[j];
                break;
            }
        }

        char customer_name[NAME_LEN * 2] = "Unknown";
        if (so->customer_idx >= 0 && so->customer_idx < s->customer_count) {
            Customer *c = &s->customers[so->customer_idx];
            sprintf(customer_name, "%s %s", c->first_name, c->last_name);
        }

        GtkTreeIter iter;
        gtk_list_store_append(store_list, &iter);
        gtk_list_store_set(store_list, &iter,
                          0, so->product_sku,
                          1, p ? p->name : so->product_sku,
                          2, customer_name,
                          3, so->qty_ordered,
                          4, so->order_date,
                          5, so->comments,
                          -1);
    }

    gtk_container_add(GTK_CONTAINER(sw), GTK_WIDGET(tree));
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void special_orders_not_received_report(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Special Order Items Not Yet Received",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Close", GTK_RESPONSE_CLOSE,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

    GtkListStore *store_list = gtk_list_store_new(6,
                                                   G_TYPE_STRING,  // SKU
                                                   G_TYPE_STRING,  // Product
                                                   G_TYPE_STRING,  // Customer
                                                   G_TYPE_INT,     // Qty
                                                   G_TYPE_STRING,  // Order Date
                                                   G_TYPE_STRING); // Comments

    GtkTreeView *tree = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_list)));

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(tree, -1, "SKU", renderer, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Product", renderer, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Customer", renderer, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Qty", renderer, "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Order Date", renderer, "text", 4, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Comments", renderer, "text", 5, NULL);

    // Populate list with ON_ORDER special orders (not yet received)
    for (int i = 0; i < s->special_order_count; i++) {
        SpecialOrder *so = &s->special_orders[i];
        if (so->status != SO_STATUS_ON_ORDER) continue;

        Product *p = NULL;
        for (int j = 0; j < s->product_count; j++) {
            if (strcmp(s->products[j].sku, so->product_sku) == 0) {
                p = &s->products[j];
                break;
            }
        }

        char customer_name[NAME_LEN * 2] = "Unknown";
        if (so->customer_idx >= 0 && so->customer_idx < s->customer_count) {
            Customer *c = &s->customers[so->customer_idx];
            sprintf(customer_name, "%s %s", c->first_name, c->last_name);
        }

        GtkTreeIter iter;
        gtk_list_store_append(store_list, &iter);
        gtk_list_store_set(store_list, &iter,
                          0, so->product_sku,
                          1, p ? p->name : so->product_sku,
                          2, customer_name,
                          3, so->qty_ordered,
                          4, so->order_date,
                          5, so->comments,
                          -1);
    }

    gtk_container_add(GTK_CONTAINER(sw), GTK_WIDGET(tree));
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void special_orders_received_report(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Special Order Items Received",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Close", GTK_RESPONSE_CLOSE,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

    GtkListStore *store_list = gtk_list_store_new(7,
                                                   G_TYPE_STRING,  // SKU
                                                   G_TYPE_STRING,  // Product
                                                   G_TYPE_STRING,  // Customer
                                                   G_TYPE_INT,     // Qty
                                                   G_TYPE_STRING,  // Order Date
                                                   G_TYPE_STRING,  // Received Date
                                                   G_TYPE_STRING); // Comments

    GtkTreeView *tree = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_list)));

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(tree, -1, "SKU", renderer, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Product", renderer, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Customer", renderer, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Qty", renderer, "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Order Date", renderer, "text", 4, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Received Date", renderer, "text", 5, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Comments", renderer, "text", 6, NULL);

    // Populate list with RECEIVED special orders
    for (int i = 0; i < s->special_order_count; i++) {
        SpecialOrder *so = &s->special_orders[i];
        if (so->status != SO_STATUS_RECEIVED && so->status != SO_STATUS_COMPLETE) continue;

        Product *p = NULL;
        for (int j = 0; j < s->product_count; j++) {
            if (strcmp(s->products[j].sku, so->product_sku) == 0) {
                p = &s->products[j];
                break;
            }
        }

        char customer_name[NAME_LEN * 2] = "Unknown";
        if (so->customer_idx >= 0 && so->customer_idx < s->customer_count) {
            Customer *c = &s->customers[so->customer_idx];
            sprintf(customer_name, "%s %s", c->first_name, c->last_name);
        }

        GtkTreeIter iter;
        gtk_list_store_append(store_list, &iter);
        gtk_list_store_set(store_list, &iter,
                          0, so->product_sku,
                          1, p ? p->name : so->product_sku,
                          2, customer_name,
                          3, so->qty_ordered,
                          4, so->order_date,
                          5, so->received_date,
                          6, so->comments,
                          -1);
    }

    gtk_container_add(GTK_CONTAINER(sw), GTK_WIDGET(tree));
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void view_special_orders_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Special Ordered Items",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Close", GTK_RESPONSE_CLOSE,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

    GtkListStore *store_list = gtk_list_store_new(7,
                                                   G_TYPE_STRING,  // Product
                                                   G_TYPE_STRING,  // Customer
                                                   G_TYPE_INT,     // Qty
                                                   G_TYPE_STRING,  // Status
                                                   G_TYPE_STRING,  // Order Date
                                                   G_TYPE_STRING,  // Comments
                                                   G_TYPE_INT);    // SO Index

    GtkTreeView *tree = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_list)));

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Product", renderer, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Customer", renderer, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Qty", renderer, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Status", renderer, "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Order Date", renderer, "text", 4, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Comments", renderer, "text", 5, NULL);

    // Populate list
    for (int i = 0; i < s->special_order_count; i++) {
        SpecialOrder *so = &s->special_orders[i];
        Product *p = NULL;
        for (int j = 0; j < s->product_count; j++) {
            if (strcmp(s->products[j].sku, so->product_sku) == 0) {
                p = &s->products[j];
                break;
            }
        }

        char customer_name[NAME_LEN * 2] = "Unknown";
        if (so->customer_idx >= 0 && so->customer_idx < s->customer_count) {
            Customer *c = &s->customers[so->customer_idx];
            sprintf(customer_name, "%s %s", c->first_name, c->last_name);
        }

        GtkTreeIter iter;
        gtk_list_store_append(store_list, &iter);
        gtk_list_store_set(store_list, &iter,
                          0, p ? p->name : so->product_sku,
                          1, customer_name,
                          2, so->qty_ordered,
                          3, so_status_names[so->status],
                          4, so->order_date,
                          5, so->comments,
                          6, i,
                          -1);
    }

    gtk_container_add(GTK_CONTAINER(sw), GTK_WIDGET(tree));

    // Buttons
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 0);

    GtkWidget *received_btn = gtk_button_new_with_label("Mark as Received");
    g_signal_connect(received_btn, "clicked", G_CALLBACK(mark_special_orders_received), store_list);
    gtk_box_pack_start(GTK_BOX(button_box), received_btn, FALSE, FALSE, 0);

    GtkWidget *notify_btn = gtk_button_new_with_label("Send Notification");
    g_signal_connect(notify_btn, "clicked", G_CALLBACK(send_special_order_notifications), store_list);
    gtk_box_pack_start(GTK_BOX(button_box), notify_btn, FALSE, FALSE, 0);

    gtk_widget_set_size_request(dialog, 800, 400);
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void view_layaways_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Layaway Transactions",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Close", GTK_RESPONSE_CLOSE,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

    GtkListStore *store_list = gtk_list_store_new(6,
                                                   G_TYPE_STRING,  // Transaction ID
                                                   G_TYPE_STRING,  // Customer
                                                   G_TYPE_DOUBLE,  // Total
                                                   G_TYPE_DOUBLE,  // Paid
                                                   G_TYPE_DOUBLE,  // Remaining
                                                   G_TYPE_INT);    // Transaction Index

    GtkTreeView *tree = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_list)));

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Transaction ID", renderer, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Customer", renderer, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Total", renderer, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Paid", renderer, "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Remaining", renderer, "text", 4, NULL);

    // Populate list with layaway transactions
    for (int i = 0; i < s->sales_count; i++) {
        Transaction *txn = &s->sales[i];
        if (txn->status == 0) { // layaway
            char customer_name[NAME_LEN * 2] = "Unknown";
            if (txn->customer_idx >= 0 && txn->customer_idx < s->customer_count) {
                Customer *c = &s->customers[txn->customer_idx];
                sprintf(customer_name, "%s %s", c->first_name, c->last_name);
            }

            double remaining = txn->total - txn->amount_paid;

            GtkTreeIter iter;
            gtk_list_store_append(store_list, &iter);
            gtk_list_store_set(store_list, &iter,
                              0, txn->transaction_id,
                              1, customer_name,
                              2, txn->total,
                              3, txn->amount_paid,
                              4, remaining,
                              5, i,
                              -1);
        }
    }

    gtk_container_add(GTK_CONTAINER(sw), GTK_WIDGET(tree));

    gtk_widget_set_size_request(dialog, 700, 400);
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void special_order_notification_dialog(SpecialOrder *so) {
    if (!so) return;

    // Find the store and customer
    Store *store = NULL;
    for (int i = 0; i < store_count; i++) {
        for (int j = 0; j < stores[i].special_order_count; j++) {
            if (&stores[i].special_orders[j] == so) {
                store = &stores[i];
                break;
            }
        }
        if (store) break;
    }
    if (!store) return;

    Customer *customer = NULL;
    if (so->customer_idx >= 0 && so->customer_idx < store->customer_count) {
        customer = &store->customers[so->customer_idx];
    }

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Special Order Notification",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Send", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    // Find product name
    char product_name[NAME_LEN] = "";
    for (int i = 0; i < store->product_count; i++) {
        if (strcmp(store->products[i].sku, so->product_sku) == 0) {
            strcpy(product_name, store->products[i].name);
            break;
        }
    }

    char info[500];
    sprintf(info, "Special Order Notification\n\nProduct: %s\nSKU: %s\nQuantity: %d\nStatus: Received\nOrder Date: %s\nReceived Date: %s\nComments: %s",
            product_name, so->product_sku, so->qty_ordered, so->order_date, so->received_date, so->comments);
    GtkWidget *info_label = gtk_label_new(info);
    gtk_box_pack_start(GTK_BOX(vbox), info_label, FALSE, FALSE, 0);

    // Customer info
    if (customer) {
        char cust_info[200];
        sprintf(cust_info, "\nCustomer: %s %s\nPreferred Contact: %s",
                customer->first_name, customer->last_name,
                customer->preferred_contact == 0 ? "Email" :
                customer->preferred_contact == 1 ? "Phone" : "Text");
        GtkWidget *cust_label = gtk_label_new(cust_info);
        gtk_box_pack_start(GTK_BOX(vbox), cust_label, FALSE, FALSE, 0);
    }

    // Notification methods with preferred method pre-selected
    GtkWidget *email_cb = gtk_check_button_new_with_label("Send Email");
    GtkWidget *phone_cb = gtk_check_button_new_with_label("Call Phone");
    GtkWidget *text_cb = gtk_check_button_new_with_label("Send Text Message");

    // Auto-select preferred contact method
    if (customer) {
        if (customer->preferred_contact == 0) { // Email
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(email_cb), TRUE);
        } else if (customer->preferred_contact == 1) { // Phone
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(phone_cb), TRUE);
        } else { // Text
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(text_cb), TRUE);
        }
    }

    gtk_box_pack_start(GTK_BOX(vbox), email_cb, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), phone_cb, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), text_cb, FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        // Mark as notified and update status to COMPLETE
        so->status = SO_STATUS_COMPLETE;
        show_info_dialog("Notification sent successfully. Special order marked as completed.");
        save_data();
    }

    gtk_widget_destroy(dialog);
}

static void mark_special_orders_received(GtkWidget *widget, gpointer data) {
    GtkListStore *store_list = GTK_LIST_STORE(data);
    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store_list), &iter);
    int received_count = 0;

    while (valid) {
        int so_index;
        gtk_tree_model_get(GTK_TREE_MODEL(store_list), &iter, 6, &so_index, -1);

        // Find the store containing this special order
        Store *store = NULL;
        for (int i = 0; i < store_count; i++) {
            if (so_index < stores[i].special_order_count) {
                store = &stores[i];
                break;
            }
            so_index -= stores[i].special_order_count;
        }

        if (store && so_index >= 0 && so_index < store->special_order_count) {
            SpecialOrder *so = &store->special_orders[so_index];
            if (so->status == SO_STATUS_ON_ORDER) {
                so->status = SO_STATUS_RECEIVED;
                // Set received date to today
                time_t now = time(NULL);
                struct tm *tm_info = localtime(&now);
                strftime(so->received_date, NAME_LEN, "%Y-%m-%d", tm_info);
                received_count++;
            }
        }

        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store_list), &iter);
    }

    if (received_count > 0) {
        char msg[128];
        sprintf(msg, "%d item(s) marked as received.", received_count);
        show_info_dialog(msg);
        save_data();
    } else {
        show_info_dialog("No items were marked as received.");
    }
}

static void send_special_order_notifications(GtkWidget *widget, gpointer data) {
    GtkListStore *store_list = GTK_LIST_STORE(data);
    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store_list), &iter);
    int notified_count = 0;

    while (valid) {
        int so_index;
        gtk_tree_model_get(GTK_TREE_MODEL(store_list), &iter, 6, &so_index, -1);

        // Find the store containing this special order
        Store *store = NULL;
        for (int i = 0; i < store_count; i++) {
            if (so_index < stores[i].special_order_count) {
                store = &stores[i];
                break;
            }
            so_index -= stores[i].special_order_count;
        }

        if (store && so_index >= 0 && so_index < store->special_order_count) {
            SpecialOrder *so = &store->special_orders[so_index];
            if (so->status == SO_STATUS_RECEIVED) {
                special_order_notification_dialog(so);
                notified_count++;
            }
        }

        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store_list), &iter);
    }

    if (notified_count == 0) {
        show_info_dialog("No received items to notify about.");
    }
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_window), "Ascend Retail Platform");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 500, 400);
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(main_window, "key-press-event", G_CALLBACK(on_main_window_key_press), NULL);

    show_main_menu();

    gtk_main();

    return 0;
}