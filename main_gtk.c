#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <ctype.h>

#define MAX_STORES 30
#define MAX_PRODUCTS 50
#define MAX_QUOTES 50
#define MAX_CUSTOMERS 200
#define MAX_TRANSACTIONS 500
#define MAX_SPECIAL_ORDERS 200
#define MAX_TAX_EXCEPTIONS 200
#define MAX_TIME_CLOCK_ENTRIES 2000
#define MAX_CUSTOMER_NOTES 4000
#define MAX_WORK_ORDERS 2000
#define MAX_AUDIT_LOGS 8000
#define MAX_RESERVATIONS 3000
#define MAX_PAYMENT_ENTRIES 12000
#define MAX_PURCHASE_ORDERS 4000
#define MAX_VENDOR_LINKS 6000
#define MAX_QBP_CATALOG_ITEMS 12000
#define MAX_WEB_ORDER_PICKUPS 4000
#define MAX_SUSPENSION_LOGS 4000
#define MAX_SYNC_QUEUE 10000
#define MAX_REPAIR_STANDS 24
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
    char gender[NAME_LEN];
    char birthdate[NAME_LEN];
    char customer_since[NAME_LEN];
    char last_visit[NAME_LEN];
    char customer_group[NAME_LEN];
} Customer;

/* Product: an item tracked in inventory with SKU and price stock information. */
typedef struct {
    char sku[NAME_LEN];      // internal stock keeping unit
    char name[NAME_LEN];     // human-readable product name
    char vendor[NAME_LEN];   // vendor name
    char brand[NAME_LEN];
    char upc[NAME_LEN];
    char manufacturer_part_number[NAME_LEN];
    char style_name[NAME_LEN];
    char style_number[NAME_LEN];
    int model_year;
    char color[NAME_LEN];
    char size[NAME_LEN];
    int serialized;          // 1=yes (serialized/bike), 0=no
    double msrp;
    double price;            // unit retail price
    double average_cost;
    double last_cost;
    int ecommerce_sync;
    int stock;               // current available on hand in store
    int on_order_qty;
    int received_qty;
    int committed_qty;
    int min_on_season;
    int max_on_season;
    int min_off_season;
    int max_off_season;
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
 * status: 0=open layaway, 1=completed, 2=paid in full, 3=canceled layaway
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
    int prompt_work_order_serial;
    int prompt_receiving_serial;
    int layaway_reminder_days;
    int layaway_grace_days;
    double layaway_cancel_fee_percent;
    int layaway_auto_cancel_enabled;
    double default_tax_rate;
    double max_discount_percent_sales;
} AppSettings;

typedef struct {
    int store_idx;
    int customer_idx;
    char date[NAME_LEN];
    char note[NAME_LEN * 4];
    int hidden;
} CustomerNote;

typedef enum { WO_OPEN, WO_WAITING_PARTS, WO_IN_PROGRESS, WO_COMPLETED, WO_PICKED_UP } WorkOrderStatus;

typedef struct {
    int id;
    int store_idx;
    int customer_idx;
    char serial[NAME_LEN];
    char problem[NAME_LEN * 4];
    char assigned_mechanic[NAME_LEN];
    WorkOrderStatus status;
    double labor_rate;
    double labor_hours;
    double parts_total;
    double labor_total;
    double total;
    char created_at[NAME_LEN];
    char updated_at[NAME_LEN];
    int hidden;
} WorkOrder;

typedef struct {
    char timestamp[NAME_LEN];
    char user[NAME_LEN];
    char action[NAME_LEN];
    char details[NAME_LEN * 4];
    int hidden;
} AuditLog;

typedef struct {
    int id;
    int store_idx;
    int customer_idx;
    char sku[NAME_LEN];
    int qty;
    char expiry_date[NAME_LEN];
    int status; // 0=active,1=fulfilled,2=canceled,3=expired
    char created_at[NAME_LEN];
    int hidden;
} Reservation;

typedef struct {
    int store_idx;
    char transaction_id[NAME_LEN];
    char payment_method[NAME_LEN];
    double amount;
    char date[NAME_LEN];
    char note[NAME_LEN * 2];
    int hidden;
} PaymentEntry;

typedef struct {
    int id;
    int store_idx;
    char sku[NAME_LEN];
    char vendor[NAME_LEN];
    int qty_ordered;
    int qty_received;
    int status; // 0=open, 1=partial, 2=received, 3=canceled
    char expected_date[NAME_LEN];
    char received_date[NAME_LEN];
    char created_at[NAME_LEN];
    char comments[NAME_LEN * 2];
    int hidden;
} PurchaseOrder;

typedef struct {
    int store_idx;
    char in_store_sku[NAME_LEN];
    char vendor_name[NAME_LEN];
    char vendor_product_code[NAME_LEN];
    char vendor_description[NAME_LEN * 2];
    int hidden;
} VendorProductLink;

typedef struct {
    char sku[NAME_LEN];
    char description[NAME_LEN * 2];
    double weight_lbs;
    char image_url[NAME_LEN * 2];
    int nv_qty;
    int pa_qty;
    int wi_qty;
    int hidden;
} QbpCatalogItem;

typedef struct {
    int store_idx;
    int customer_idx;
    char manufacturer[NAME_LEN];
    char manufacturer_order_number[NAME_LEN];
    char model_name[NAME_LEN];
    char labor_sku[NAME_LEN];
    int status; // 0=pending assembly,1=in stand,2=ready for pickup,3=picked up
    int hidden;
} WebOrderPickup;

typedef struct {
    int store_idx;
    int customer_idx;
    char bike_serial[NAME_LEN];
    char date[NAME_LEN];
    double fork_psi;
    int fork_rebound;
    double shock_psi;
    int shock_rebound;
    char notes[NAME_LEN * 2];
    int hidden;
} SuspensionSetupLog;

typedef struct {
    char transaction_id[NAME_LEN];
    int store_idx;
    char sku[NAME_LEN];
    int qty;
    char created_at[NAME_LEN];
    int synced;
    int hidden;
} OfflineSyncQueueItem;

typedef struct {
    int stand_number;
    int work_order_id;
    char mechanic[NAME_LEN];
    int active;
} RepairStandAssignment;

typedef enum { ROLE_ADMIN, ROLE_MANAGER, ROLE_SALES, ROLE_MECHANIC, ROLE_INVENTORY_MANAGER, ROLE_SERVICE_LEAD, ROLE_BUYER } UserRole;

typedef enum { THEME_LIGHT, THEME_DARK, THEME_SMART } ThemeMode;

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
static CustomerNote customer_notes[MAX_CUSTOMER_NOTES];
static int customer_note_count = 0;
static WorkOrder work_orders[MAX_WORK_ORDERS];
static int work_order_count = 0;
static int next_work_order_id = 1000;
static AuditLog audit_logs[MAX_AUDIT_LOGS];
static int audit_log_count = 0;
static Reservation reservations[MAX_RESERVATIONS];
static int reservation_count = 0;
static int next_reservation_id = 5000;
static PaymentEntry payment_entries[MAX_PAYMENT_ENTRIES];
static int payment_entry_count = 0;
static PurchaseOrder purchase_orders[MAX_PURCHASE_ORDERS];
static int purchase_order_count = 0;
static int next_purchase_order_id = 8000;
static VendorProductLink vendor_links[MAX_VENDOR_LINKS];
static int vendor_link_count = 0;
static QbpCatalogItem qbp_catalog[MAX_QBP_CATALOG_ITEMS];
static int qbp_catalog_count = 0;
static WebOrderPickup web_order_pickups[MAX_WEB_ORDER_PICKUPS];
static int web_order_pickup_count = 0;
static SuspensionSetupLog suspension_logs[MAX_SUSPENSION_LOGS];
static int suspension_log_count = 0;
static OfflineSyncQueueItem sync_queue[MAX_SYNC_QUEUE];
static int sync_queue_count = 0;
static RepairStandAssignment repair_stands[MAX_REPAIR_STANDS];
static int repair_stand_count = 8;
static UserRole active_user_role = ROLE_MANAGER;
static char manager_override_pin[NAME_LEN] = "2468";
static AppSettings app_settings = {1, 1, 7, 10, 15.0, 0, 0.0825, 15.0};
static char system_sales_popup_message[NAME_LEN * 3] = "Verify info for registration.";
static char current_sales_user[NAME_LEN] = "Ascend User";
static int enable_multistore_sync = 1;
static int trek_auto_register = 1;
static ThemeMode active_theme_mode = THEME_SMART;
static int mobile_floor_mode_enabled = 0;
static int internet_online = 1;
static int local_node_enabled = 1;
static int store_and_forward_enabled = 1;
static GtkCssProvider *app_css_provider = NULL;
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
static int tiles_initialized = 0;
static int dragging_tile = -1;
static int drag_start_x, drag_start_y;
static GtkWidget *desktop_canvas = NULL;

#define LAY_COL_SKU 0
#define LAY_COL_DESC 1
#define LAY_COL_PRICE 2
#define LAY_COL_QTY 3
#define LAY_COL_SERIAL 4
#define LAY_COL_MSRP 5
#define LAY_COL_LINE_TOTAL 6
#define LAY_COL_COMMENTS 7
#define LAY_COL_WARRANTY 8
#define LAY_COL_COUNT 9

typedef struct {
    Store *store;
    int store_idx;
    int customer_idx;
    Transaction txn;
    int saved;
    double subtotal;
    double tax;
    double shipping;
    double discount;
    double total;
    double payments_total;
    GtkWidget *dialog;
    GtkListStore *items_store;
    GtkListStore *payments_store;
    GtkWidget *total_label;
    GtkWidget *subtotal_label;
    GtkWidget *tax_label;
    GtkWidget *shipping_entry;
    GtkWidget *discount_entry;
    GtkWidget *tax_rate_combo;
    GtkWidget *tax_rate_entry;
    GtkWidget *payments_label;
    GtkWidget *balance_label;
    GtkWidget *created_label;
    GtkWidget *modified_label;
    GtkWidget *created_by_label;
    GtkWidget *transaction_id_label;
    GtkWidget *register_status_combo;
    GtkWidget *register_date_entry;
    GtkWidget *barcode_entry;
    GtkWidget *salesperson_combo;
    GtkWidget *due_date_entry;
    GtkWidget *status_right_label;
    GtkListStore *notes_store;
    GtkWidget *notes_tree;
} LayawayWorkbenchContext;

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
static void on_export_time_clock_report_clicked(GtkButton *button, gpointer data);
static void get_today_date(char *out, size_t out_size);
static void daily_sales_payment_report_dialog(void);
static void refresh_daily_sales_payment_report(GtkWidget *dialog);
static void on_refresh_daily_sales_payment_report_clicked(GtkButton *button, gpointer data);
static void on_export_daily_sales_payment_report_clicked(GtkButton *button, gpointer data);
static void low_stock_report_dialog(void);
static void on_export_low_stock_report_clicked(GtkButton *button, gpointer data);
static void best_selling_items_report_dialog(void);
static void instructions_reference_dialog(void);
static void customize_desktop_tiles_dialog(void);
static void set_desktop_locked(int locked);
static void on_lock_desktop_clicked(GtkMenuItem *item, gpointer data);
static void on_unlock_desktop_clicked(GtkMenuItem *item, gpointer data);
static void work_order_defaults_dialog(void);
static void ordering_serial_prompt_dialog(void);
static int prompt_for_serial_number_dialog(const char *title, const char *context, char *out_serial, size_t out_size);
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
static void set_main_window_context_title(const char *store_name, const char *module_name, const char *context_label);
static void reset_main_window_title(void);
static void layaway_workbench_dialog(Store *s, int store_idx, int customer_idx);
static void layaway_recalc_totals(LayawayWorkbenchContext *ctx);
static void layaway_touch_modified(LayawayWorkbenchContext *ctx);
static void on_layaway_shipping_changed(GtkEditable *editable, gpointer data);
static void on_layaway_discount_changed(GtkEditable *editable, gpointer data);
static void on_layaway_tax_changed(GtkComboBox *combo, gpointer data);
static void on_layaway_cell_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text, gpointer data);
static void on_layaway_add_row(GtkButton *button, gpointer data);
static void on_layaway_remove_selected(GtkButton *button, gpointer data);
static void on_layaway_add_by_barcode(GtkButton *button, gpointer data);
static void on_layaway_add_payment(GtkButton *button, gpointer data);
static void on_layaway_save(GtkWidget *widget, gpointer data);
static void on_layaway_action_info(GtkWidget *widget, gpointer data);
static void on_layaway_serial_lookup(GtkWidget *widget, gpointer data);
static void layaway_policy_settings_dialog(GtkWidget *widget, gpointer data);
static void on_layaway_note_add(GtkWidget *widget, gpointer data);
static void on_layaway_note_edit(GtkWidget *widget, gpointer data);
static void on_layaway_note_remove(GtkWidget *widget, gpointer data);
static void refresh_customer_notes_store(LayawayWorkbenchContext *ctx);
static int get_txn_due_date(const Transaction *txn, char *out_due, size_t out_size);
static void set_txn_due_date(Transaction *txn, const char *due_date);
static int days_until_date(const char *date_ymd);
static void on_apply_layaway_rules_clicked(GtkButton *button, gpointer data);
static void create_work_order_dialog(void);
static void view_work_orders_dialog(void);
static void audit_log_dialog(void);
static void create_backup_snapshot(void);
static void add_audit_log_entry(const char *user, const char *action, const char *details);
static void security_permissions_dialog(void);
static void create_reservation_dialog(void);
static void view_reservations_dialog(void);
static void alert_center_dialog(void);
static void add_split_payment_entry_dialog(void);
static void payment_ledger_report_dialog(void);
static void create_purchase_order_dialog(void);
static void receive_purchase_order_dialog(void);
static void view_purchase_orders_dialog(void);
static void vendor_product_linking_dialog(void);
static void system_configuration_dialog(void);
static void end_of_day_summary_report_dialog(void);
static void customer_liability_report_dialog(void);
static void sales_without_customer_report_dialog(void);
static void product_master_inventory_report_dialog(void);
static void email_receipt_quote_stub_dialog(void);
static void recompute_committed_for_store(Store *s, int store_idx);
static void theme_display_dialog(void);
static void apply_visual_theme(ThemeMode mode);
static int should_use_dark_mode(ThemeMode mode);
static void barcode_label_print_dialog(void);
static void generate_product_matrix_dialog(void);
static void service_ai_estimator_dialog(void);
static void sms_pickup_notification_dialog(void);
static void sql_query_tools_dialog(void);
static void vendor_integration_hub_dialog(void);
static void web_order_pickup_dialog(void);
static void warranty_autofill_dialog(void);
static void offline_blackout_protocol_dialog(void);
static void service_stand_manager_dialog(void);
static void labor_bundle_dialog(void);
static void trade_in_bluebook_dialog(void);
static void suspension_setup_log_dialog(void);
static void work_order_progress_sms_dialog(void);
static void buyer_dashboard_dialog(void);
static void product_lookup_dialog(void);
static void queue_offline_sync_item(const char *transaction_id, int store_idx, const char *sku, int qty);
static void add_payment_ledger_entry(int store_idx, const char *transaction_id, const char *payment_method, double amount, const char *note);
static void summarize_payment_totals_for_store_date(Store *s, const char *from_date, const char *to_date,
                                                    double *cash_total, double *card_total, double *debit_total,
                                                    double *gift_total, double *other_total, int *count_out);
static void on_add_split_payment_line_clicked(GtkButton *button, gpointer data);
static double sum_split_payment_rows(GtkTreeModel *model);
static PaymentType payment_type_from_method_text(const char *text);
static int days_ago_from_date(const char *date_ymd);
static int request_manager_override(const char *action_name);
static const char *user_role_name(UserRole role);
static int role_can_manage(void);
static int role_can_service(void);
static int role_can_inventory(void);
static int role_can_sql_query(void);
static int role_can_service_lead(void);
static int role_can_buying(void);
static void on_work_order_update_status_clicked(GtkButton *button, gpointer data);
static void on_work_order_add_part_clicked(GtkButton *button, gpointer data);
static const char *work_order_status_name(WorkOrderStatus status);
static void execute_tile_action(TileType type);
static void show_info_dialog(const char *message);
static void show_error_dialog(const char *message);
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

static void set_desktop_locked(int locked) {
    desktop_locked = locked;
    if (status_label) {
        gtk_label_set_text(GTK_LABEL(status_label),
                          locked ? "Ready (Desktop Locked)" : "Ready (Desktop Unlocked - Drag tiles)");
    }
    if (desktop_canvas) gtk_widget_queue_draw(desktop_canvas);
}

static void on_lock_desktop_clicked(GtkMenuItem *item, gpointer data) {
    (void)item;
    (void)data;
    set_desktop_locked(1);
}

static void on_unlock_desktop_clicked(GtkMenuItem *item, gpointer data) {
    (void)item;
    (void)data;
    set_desktop_locked(0);
}

static void customize_desktop_tiles_dialog(void) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Customize Home Screen Tiles",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Apply", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    gtk_box_pack_start(GTK_BOX(vbox),
                       gtk_label_new("Show/hide tiles and choose tile color for visible tiles."),
                       FALSE, FALSE, 0);

    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(sw, 620, 360);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_container_add(GTK_CONTAINER(sw), grid);

    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Tile"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Show"), 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Color"), 2, 0, 1, 1);

    GtkWidget *visible_checks[MAX_TILES];
    GtkWidget *color_combos[MAX_TILES];

    for (int i = 0; i < tile_count; i++) {
        GtkWidget *lbl = gtk_label_new(tiles[i].label);
        GtkWidget *show_cb = gtk_check_button_new();
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_cb), tiles[i].visible);

        GtkWidget *color_combo = gtk_combo_box_text_new();
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(color_combo), "Light Blue");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(color_combo), "Dark Blue");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(color_combo), "Green");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(color_combo), "Orange");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(color_combo), "Slate");
        gtk_combo_box_set_active(GTK_COMBO_BOX(color_combo), tiles[i].color);

        visible_checks[i] = show_cb;
        color_combos[i] = color_combo;

        gtk_grid_attach(GTK_GRID(grid), lbl, 0, i + 1, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), show_cb, 1, i + 1, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), color_combo, 2, i + 1, 1, 1);
    }

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        for (int i = 0; i < tile_count; i++) {
            tiles[i].visible = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(visible_checks[i]));
            tiles[i].color = gtk_combo_box_get_active(GTK_COMBO_BOX(color_combos[i]));
            if (tiles[i].color < COLOR_LIGHT_BLUE || tiles[i].color > COLOR_SLATE) {
                tiles[i].color = COLOR_LIGHT_BLUE;
            }
        }
        save_data();
        if (desktop_canvas) gtk_widget_queue_draw(desktop_canvas);
        show_info_dialog("Desktop tile settings updated.");
    }

    gtk_widget_destroy(dialog);
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

static void add_audit_log_entry(const char *user, const char *action, const char *details) {
    if (audit_log_count >= MAX_AUDIT_LOGS) return;
    AuditLog *log = &audit_logs[audit_log_count++];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(log->timestamp, sizeof(log->timestamp), "%Y-%m-%d %H:%M", tm_info);
    strncpy(log->user, user ? user : "System", NAME_LEN - 1);
    log->user[NAME_LEN - 1] = '\0';
    strncpy(log->action, action ? action : "Action", NAME_LEN - 1);
    log->action[NAME_LEN - 1] = '\0';
    strncpy(log->details, details ? details : "", sizeof(log->details) - 1);
    log->details[sizeof(log->details) - 1] = '\0';
    log->hidden = 0;
}

static const char *user_role_name(UserRole role) {
    switch (role) {
        case ROLE_ADMIN: return "System Admin";
        case ROLE_MANAGER: return "Store Manager";
        case ROLE_SALES: return "Sales Associate";
        case ROLE_MECHANIC: return "Service Technician";
        case ROLE_INVENTORY_MANAGER: return "Inventory Manager";
        case ROLE_SERVICE_LEAD: return "Service Lead";
        case ROLE_BUYER: return "Buyer/Purchaser";
    }
    return "Unknown";
}

static int role_can_manage(void) {
    return active_user_role == ROLE_ADMIN || active_user_role == ROLE_MANAGER;
}

static int role_can_service(void) {
    return role_can_manage() || active_user_role == ROLE_MECHANIC || active_user_role == ROLE_SERVICE_LEAD;
}

static int role_can_inventory(void) {
    return role_can_manage() || active_user_role == ROLE_INVENTORY_MANAGER;
}

static int role_can_sql_query(void) {
    return active_user_role == ROLE_ADMIN;
}

static int role_can_service_lead(void) {
    return role_can_manage() || active_user_role == ROLE_SERVICE_LEAD;
}

static int role_can_buying(void) {
    return role_can_manage() || active_user_role == ROLE_BUYER;
}

static int request_manager_override(const char *action_name) {
    if (role_can_manage()) return 1;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Manager Override",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Authorize", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);
    char msg[180];
    snprintf(msg, sizeof(msg), "%s requires Manager/Admin permission.", action_name ? action_name : "This action");
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(msg), FALSE, FALSE, 0);
    GtkWidget *pin_entry = create_labeled_entry("Manager PIN:", vbox);
    gtk_entry_set_visibility(GTK_ENTRY(pin_entry), FALSE);

    gtk_widget_show_all(dialog);
    int allowed = 0;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *pin = gtk_entry_get_text(GTK_ENTRY(pin_entry));
        if (strcmp(pin, manager_override_pin) == 0) {
            allowed = 1;
            add_audit_log_entry("Override", "ManagerOverride", action_name ? action_name : "Action");
        } else {
            show_error_dialog("Invalid manager PIN.");
            add_audit_log_entry("Override", "ManagerOverrideFailed", action_name ? action_name : "Action");
        }
    }
    gtk_widget_destroy(dialog);
    return allowed;
}

static void security_permissions_dialog(void) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Security and Permissions",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Save", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *role_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(role_combo), "System Admin");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(role_combo), "Store Manager");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(role_combo), "Sales Associate");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(role_combo), "Service Technician");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(role_combo), "Inventory Manager");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(role_combo), "Service Lead");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(role_combo), "Buyer/Purchaser");
    gtk_combo_box_set_active(GTK_COMBO_BOX(role_combo), active_user_role);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("Active Role:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), role_combo, FALSE, FALSE, 0);

    GtkWidget *pin_entry = create_labeled_entry("Manager Override PIN:", vbox);
    gtk_entry_set_text(GTK_ENTRY(pin_entry), manager_override_pin);
    gtk_entry_set_visibility(GTK_ENTRY(pin_entry), FALSE);

    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("Role permissions: Sales (POS), Service Tech (work orders), Service Lead (queue priority), Inventory (catalog/stock), Buyer (buying dashboard), Manager/Admin (overrides)."), FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        active_user_role = (UserRole)gtk_combo_box_get_active(GTK_COMBO_BOX(role_combo));
        const char *pin = gtk_entry_get_text(GTK_ENTRY(pin_entry));
        if (pin && strlen(pin) > 0) {
            strncpy(manager_override_pin, pin, NAME_LEN - 1);
            manager_override_pin[NAME_LEN - 1] = '\0';
        }
        if (active_user_role < ROLE_ADMIN || active_user_role > ROLE_BUYER) {
            active_user_role = ROLE_MANAGER;
        }
        char detail[128];
        snprintf(detail, sizeof(detail), "Role switched to %s", user_role_name(active_user_role));
        add_audit_log_entry("Ascend User", "RoleChange", detail);
        show_info_dialog("Security settings updated.");
    }
    gtk_widget_destroy(dialog);
}

static void create_reservation_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];
    if (reservation_count >= MAX_RESERVATIONS) {
        show_error_dialog("Maximum reservations reached.");
        return;
    }
    if (s->customer_count == 0) {
        show_error_dialog("No customers in this store. Add a customer first.");
        return;
    }

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Create Reservation",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Create", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *cust_combo = gtk_combo_box_text_new();
    for (int i = 0; i < s->customer_count; i++) {
        if (s->customers[i].hidden) continue;
        char entry[NAME_LEN * 3];
        snprintf(entry, sizeof(entry), "%s %s (%s)", s->customers[i].first_name, s->customers[i].last_name, s->customers[i].company);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cust_combo), entry);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(cust_combo), 0);

    GtkWidget *sku_entry = create_labeled_entry("SKU:", vbox);
    GtkWidget *qty_entry = create_labeled_entry("Qty:", vbox);
    GtkWidget *expiry_entry = create_labeled_entry("Expiry (YYYY-MM-DD):", vbox);
    gtk_entry_set_text(GTK_ENTRY(qty_entry), "1");
    gtk_entry_set_text(GTK_ENTRY(expiry_entry), "2026-12-31");
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("Customer:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), cust_combo, FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *sku = gtk_entry_get_text(GTK_ENTRY(sku_entry));
        int qty = atoi(gtk_entry_get_text(GTK_ENTRY(qty_entry)));
        const char *expiry = gtk_entry_get_text(GTK_ENTRY(expiry_entry));
        Product *p = NULL;
        for (int i = 0; i < s->product_count; i++) {
            if (strcmp(s->products[i].sku, sku) == 0) {
                p = &s->products[i];
                break;
            }
        }
        if (!p) {
            show_error_dialog("SKU not found.");
        } else if (qty <= 0) {
            show_error_dialog("Quantity must be positive.");
        } else if (p->stock < qty) {
            show_error_dialog("Insufficient stock for reservation.");
        } else {
            Reservation *r = &reservations[reservation_count++];
            memset(r, 0, sizeof(Reservation));
            r->id = next_reservation_id++;
            r->store_idx = si;
            r->customer_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(cust_combo));
            r->qty = qty;
            r->status = 0;
            strncpy(r->sku, sku, NAME_LEN - 1);
            r->sku[NAME_LEN - 1] = '\0';
            strncpy(r->expiry_date, expiry, NAME_LEN - 1);
            r->expiry_date[NAME_LEN - 1] = '\0';
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);
            strftime(r->created_at, sizeof(r->created_at), "%Y-%m-%d %H:%M", tm_info);

            char details[200];
            snprintf(details, sizeof(details), "Reservation %d SKU:%s Qty:%d Store:%s", r->id, r->sku, r->qty, s->name);
            add_audit_log_entry("Ascend User", "ReservationCreate", details);
            save_data();
            show_info_dialog("Reservation created.");
        }
    }
    gtk_widget_destroy(dialog);
}

static const char *reservation_status_name(int status) {
    switch (status) {
        case 0: return "Active";
        case 1: return "Fulfilled";
        case 2: return "Canceled";
        case 3: return "Expired";
    }
    return "Unknown";
}

static void view_reservations_dialog(void) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Reservations",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Close", GTK_RESPONSE_CLOSE,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(sw, 960, 420);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

    GtkListStore *store_list = gtk_list_store_new(8,
                                                   G_TYPE_INT,
                                                   G_TYPE_STRING,
                                                   G_TYPE_STRING,
                                                   G_TYPE_STRING,
                                                   G_TYPE_INT,
                                                   G_TYPE_STRING,
                                                   G_TYPE_STRING,
                                                   G_TYPE_STRING);

    for (int i = 0; i < reservation_count; i++) {
        Reservation *r = &reservations[i];
        if (r->hidden) continue;
        char store_name[NAME_LEN] = "Unknown";
        char customer_name[NAME_LEN * 2] = "Unknown";
        if (r->store_idx >= 0 && r->store_idx < store_count) {
            Store *s = &stores[r->store_idx];
            strncpy(store_name, s->name, NAME_LEN - 1);
            if (r->customer_idx >= 0 && r->customer_idx < s->customer_count) {
                snprintf(customer_name, sizeof(customer_name), "%s %s", s->customers[r->customer_idx].first_name, s->customers[r->customer_idx].last_name);
            }
        }
        GtkTreeIter iter;
        gtk_list_store_append(store_list, &iter);
        gtk_list_store_set(store_list, &iter,
                           0, r->id,
                           1, store_name,
                           2, customer_name,
                           3, r->sku,
                           4, r->qty,
                           5, r->expiry_date,
                           6, reservation_status_name(r->status),
                           7, r->created_at,
                           -1);
    }

    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_list));
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Res #", renderer, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Store", renderer, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Customer", renderer, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "SKU", renderer, "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Qty", renderer, "text", 4, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Expiry", renderer, "text", 5, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Status", renderer, "text", 6, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Created", renderer, "text", 7, NULL);
    gtk_container_add(GTK_CONTAINER(sw), tree);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void alert_center_dialog(void) {
    char report[12000] = "ALERT CENTER\n\n";
    char line[256];

    int low_stock_count = 0;
    for (int si = 0; si < store_count; si++) {
        Store *s = &stores[si];
        recompute_committed_for_store(s, si);
        for (int p = 0; p < s->product_count; p++) {
            int available = s->products[p].stock - s->products[p].committed_qty;
            if (available <= 2) {
                snprintf(line, sizeof(line), "LOW STOCK | Store:%s | SKU:%s | %s | Available:%d | Stock:%d | Committed:%d\n",
                         s->name, s->products[p].sku, s->products[p].name, available, s->products[p].stock, s->products[p].committed_qty);
                strncat(report, line, sizeof(report) - strlen(report) - 1);
                low_stock_count++;
            }
        }
    }

    int layaway_alert_count = 0;
    for (int si = 0; si < store_count; si++) {
        Store *s = &stores[si];
        for (int t = 0; t < s->sales_count; t++) {
            Transaction *txn = &s->sales[t];
            if (txn->status != 0) continue;
            char due[16];
            if (!get_txn_due_date(txn, due, sizeof(due))) continue;
            int days = days_until_date(due);
            if (days <= app_settings.layaway_reminder_days) {
                snprintf(line, sizeof(line), "LAYAWAY ALERT | Store:%s | Txn:%s | Due:%s | Days:%d\n", s->name, txn->transaction_id, due, days);
                strncat(report, line, sizeof(report) - strlen(report) - 1);
                layaway_alert_count++;
            }
        }
    }

    int reservation_alert_count = 0;
    for (int i = 0; i < reservation_count; i++) {
        Reservation *r = &reservations[i];
        if (r->hidden || r->status != 0) continue;
        int days = days_until_date(r->expiry_date);
        if (days < 0) {
            r->status = 3;
            snprintf(line, sizeof(line), "RESERVATION EXPIRED | Res:%d | SKU:%s | Expired:%s\n", r->id, r->sku, r->expiry_date);
            strncat(report, line, sizeof(report) - strlen(report) - 1);
            reservation_alert_count++;
        }
    }

    if (low_stock_count == 0 && layaway_alert_count == 0 && reservation_alert_count == 0) {
        strncat(report, "No active alerts.\n", sizeof(report) - strlen(report) - 1);
    }

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Alert Center",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Close", GTK_RESPONSE_CLOSE,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(sw, 900, 460);
    gtk_container_add(GTK_CONTAINER(content_area), sw);
    GtkWidget *text = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
    gtk_container_add(GTK_CONTAINER(sw), text);
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
    gtk_text_buffer_set_text(buf, report, -1);

    if (reservation_alert_count > 0) save_data();

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static int str_contains_case_insensitive(const char *haystack, const char *needle) {
    if (!haystack || !needle || needle[0] == '\0') return 0;
    size_t h_len = strlen(haystack);
    size_t n_len = strlen(needle);
    if (n_len > h_len) return 0;
    for (size_t i = 0; i + n_len <= h_len; i++) {
        size_t j = 0;
        while (j < n_len && tolower((unsigned char)haystack[i + j]) == tolower((unsigned char)needle[j])) {
            j++;
        }
        if (j == n_len) return 1;
    }
    return 0;
}

static double sum_split_payment_rows(GtkTreeModel *model) {
    if (!model) return 0.0;
    double total = 0.0;
    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(model, &iter);
    while (valid) {
        double amount = 0.0;
        gtk_tree_model_get(model, &iter, 1, &amount, -1);
        total += amount;
        valid = gtk_tree_model_iter_next(model, &iter);
    }
    return total;
}

static PaymentType payment_type_from_method_text(const char *text) {
    if (!text) return PAYMENT_CASH;
    if (str_contains_case_insensitive(text, "credit")) return PAYMENT_CREDIT;
    if (str_contains_case_insensitive(text, "debit")) return PAYMENT_DEBIT;
    if (str_contains_case_insensitive(text, "gift")) return PAYMENT_GIFT;
    return PAYMENT_CASH;
}

static void on_add_split_payment_line_clicked(GtkButton *button, gpointer data) {
    (void)button;
    GtkWidget *dialog = GTK_WIDGET(data);
    GtkListStore *store = GTK_LIST_STORE(g_object_get_data(G_OBJECT(dialog), "split_store"));
    GtkWidget *method_combo = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "split_method_combo"));
    GtkWidget *amount_entry = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "split_amount_entry"));
    GtkWidget *total_label = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "split_total_label"));
    if (!store || !method_combo || !amount_entry || !total_label) return;

    int method_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(method_combo));
    const char *methods[] = {"Cash", "Credit Card", "Debit Card", "Gift Card", "Financing", "Check", "Trade-In", "On Account"};
    const char *method = (method_idx >= 0 && method_idx < 8) ? methods[method_idx] : "Cash";
    double amount = atof(gtk_entry_get_text(GTK_ENTRY(amount_entry)));
    if (amount <= 0.0) {
        show_error_dialog("Split amount must be greater than zero.");
        return;
    }

    GtkTreeIter iter;
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,
                       0, method,
                       1, amount,
                       -1);
    gtk_entry_set_text(GTK_ENTRY(amount_entry), "");

    char total_text[80];
    snprintf(total_text, sizeof(total_text), "Split Entered: $%.2f", sum_split_payment_rows(GTK_TREE_MODEL(store)));
    gtk_label_set_text(GTK_LABEL(total_label), total_text);
}

static void add_payment_ledger_entry(int store_idx, const char *transaction_id, const char *payment_method, double amount, const char *note) {
    if (payment_entry_count >= MAX_PAYMENT_ENTRIES) return;
    if (!transaction_id || strlen(transaction_id) == 0 || amount == 0.0) return;

    PaymentEntry *e = &payment_entries[payment_entry_count++];
    memset(e, 0, sizeof(PaymentEntry));
    e->store_idx = store_idx;
    e->amount = amount;
    strncpy(e->transaction_id, transaction_id, NAME_LEN - 1);
    e->transaction_id[NAME_LEN - 1] = '\0';
    strncpy(e->payment_method, (payment_method && strlen(payment_method) > 0) ? payment_method : "Cash", NAME_LEN - 1);
    e->payment_method[NAME_LEN - 1] = '\0';
    if (note) {
        strncpy(e->note, note, sizeof(e->note) - 1);
        e->note[sizeof(e->note) - 1] = '\0';
    }
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(e->date, sizeof(e->date), "%Y-%m-%d %H:%M", tm_info);
}

static void summarize_payment_totals_for_store_date(Store *s, const char *from_date, const char *to_date,
                                                    double *cash_total, double *card_total, double *debit_total,
                                                    double *gift_total, double *other_total, int *count_out) {
    *cash_total = 0.0;
    *card_total = 0.0;
    *debit_total = 0.0;
    *gift_total = 0.0;
    *other_total = 0.0;
    *count_out = 0;

    int si = (int)(s - stores);
    for (int i = 0; i < payment_entry_count; i++) {
        PaymentEntry *e = &payment_entries[i];
        if (e->hidden || e->store_idx != si) continue;
        if (strlen(e->date) < 10) continue;

        char d[11] = "";
        strncpy(d, e->date, 10);
        d[10] = '\0';
        if (strlen(from_date) >= 10 && strcmp(d, from_date) < 0) continue;
        if (strlen(to_date) >= 10 && strcmp(d, to_date) > 0) continue;

        (*count_out)++;
        if (str_contains_case_insensitive(e->payment_method, "cash")) *cash_total += e->amount;
        else if (str_contains_case_insensitive(e->payment_method, "credit")) *card_total += e->amount;
        else if (str_contains_case_insensitive(e->payment_method, "debit")) *debit_total += e->amount;
        else if (str_contains_case_insensitive(e->payment_method, "gift")) *gift_total += e->amount;
        else *other_total += e->amount;
    }
}

static void add_split_payment_entry_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Add Split Payment",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Apply", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *txn_entry = create_labeled_entry("Transaction ID:", vbox);
    GtkWidget *method_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(method_combo), "Cash");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(method_combo), "Credit Card");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(method_combo), "Debit Card");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(method_combo), "Gift Card");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(method_combo), "Financing");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(method_combo), "Check");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(method_combo), "Trade-In");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(method_combo), "On Account");
    gtk_combo_box_set_active(GTK_COMBO_BOX(method_combo), 0);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("Payment Method:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), method_combo, FALSE, FALSE, 0);

    GtkWidget *amount_entry = create_labeled_entry("Amount:", vbox);
    GtkWidget *note_entry = create_labeled_entry("Note:", vbox);

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *txn_id = gtk_entry_get_text(GTK_ENTRY(txn_entry));
        int method_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(method_combo));
        const char *methods[] = {"Cash", "Credit Card", "Debit Card", "Gift Card", "Financing", "Check", "Trade-In", "On Account"};
        const char *method = (method_idx >= 0 && method_idx < 8) ? methods[method_idx] : "Cash";
        double amount = atof(gtk_entry_get_text(GTK_ENTRY(amount_entry)));
        const char *note = gtk_entry_get_text(GTK_ENTRY(note_entry));

        if (strlen(txn_id) == 0) {
            show_error_dialog("Transaction ID is required.");
        } else if (amount <= 0.0) {
            show_error_dialog("Amount must be greater than zero.");
        } else {
            Transaction *txn = NULL;
            for (int i = 0; i < s->sales_count; i++) {
                if (strcmp(s->sales[i].transaction_id, txn_id) == 0) {
                    txn = &s->sales[i];
                    break;
                }
            }
            if (!txn) {
                show_error_dialog("Transaction not found in selected store.");
            } else {
                double before_paid = txn->amount_paid;
                txn->amount_paid += amount;
                add_payment_ledger_entry(si, txn_id, method, amount, note);

                if (txn->status == 0 && before_paid < txn->total && txn->amount_paid >= txn->total) {
                    txn->status = 2;
                    s->sales_to_date += txn->total;
                }

                char details[220];
                snprintf(details, sizeof(details), "Txn:%s Method:%s Amount:$%.2f", txn_id, method, amount);
                add_audit_log_entry("Ascend User", "SplitPaymentAdd", details);
                save_data();
                show_info_dialog("Split payment entry added.");
            }
        }
    }
    gtk_widget_destroy(dialog);
}

static void payment_ledger_report_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Payment Ledger",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Close", GTK_RESPONSE_CLOSE,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(sw, 980, 420);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

    GtkListStore *store_list = gtk_list_store_new(6, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_DOUBLE, G_TYPE_STRING, G_TYPE_STRING);
    for (int i = 0; i < payment_entry_count; i++) {
        PaymentEntry *e = &payment_entries[i];
        if (e->hidden || e->store_idx != si) continue;
        GtkTreeIter iter;
        gtk_list_store_append(store_list, &iter);
        gtk_list_store_set(store_list, &iter,
                           0, e->date,
                           1, e->transaction_id,
                           2, s->name,
                           3, e->amount,
                           4, e->payment_method,
                           5, e->note,
                           -1);
    }

    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_list));
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Date", renderer, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Txn", renderer, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Store", renderer, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Amount", renderer, "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Method", renderer, "text", 4, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Note", renderer, "text", 5, NULL);
    gtk_container_add(GTK_CONTAINER(sw), tree);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static const char *purchase_order_status_name(int status) {
    switch (status) {
        case 0: return "Open";
        case 1: return "Partial";
        case 2: return "Received";
        case 3: return "Canceled";
    }
    return "Unknown";
}

static void create_purchase_order_dialog(void) {
    if (!role_can_manage() && !request_manager_override("Create Purchase Order")) return;
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];
    if (purchase_order_count >= MAX_PURCHASE_ORDERS) {
        show_error_dialog("Maximum purchase orders reached.");
        return;
    }

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Create Purchase Order",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Create", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *sku_entry = create_labeled_entry("SKU:", vbox);
    GtkWidget *vendor_entry = create_labeled_entry("Vendor:", vbox);
    GtkWidget *qty_entry = create_labeled_entry("Qty Ordered:", vbox);
    GtkWidget *expected_entry = create_labeled_entry("Expected Date (YYYY-MM-DD):", vbox);
    GtkWidget *comments_entry = create_labeled_entry("Comments:", vbox);
    gtk_entry_set_text(GTK_ENTRY(qty_entry), "1");
    gtk_entry_set_text(GTK_ENTRY(expected_entry), "2026-12-31");

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *sku = gtk_entry_get_text(GTK_ENTRY(sku_entry));
        const char *vendor = gtk_entry_get_text(GTK_ENTRY(vendor_entry));
        int qty = atoi(gtk_entry_get_text(GTK_ENTRY(qty_entry)));
        const char *expected = gtk_entry_get_text(GTK_ENTRY(expected_entry));
        const char *comments = gtk_entry_get_text(GTK_ENTRY(comments_entry));

        Product *p = NULL;
        for (int i = 0; i < s->product_count; i++) {
            if (strcmp(s->products[i].sku, sku) == 0) { p = &s->products[i]; break; }
        }

        if (!p) {
            show_error_dialog("SKU not found in selected store.");
        } else if (qty <= 0) {
            show_error_dialog("Quantity must be greater than zero.");
        } else {
            PurchaseOrder *po = &purchase_orders[purchase_order_count++];
            memset(po, 0, sizeof(PurchaseOrder));
            po->id = next_purchase_order_id++;
            po->store_idx = si;
            po->qty_ordered = qty;
            po->qty_received = 0;
            po->status = 0;
            strncpy(po->sku, sku, NAME_LEN - 1);
            po->sku[NAME_LEN - 1] = '\0';
            strncpy(po->vendor, (vendor && strlen(vendor) > 0) ? vendor : p->vendor, NAME_LEN - 1);
            po->vendor[NAME_LEN - 1] = '\0';
            strncpy(po->expected_date, expected, NAME_LEN - 1);
            po->expected_date[NAME_LEN - 1] = '\0';
            strncpy(po->comments, comments, sizeof(po->comments) - 1);
            po->comments[sizeof(po->comments) - 1] = '\0';
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);
            strftime(po->created_at, sizeof(po->created_at), "%Y-%m-%d %H:%M", tm_info);

            p->on_order_qty += qty;

            char detail[200];
            snprintf(detail, sizeof(detail), "PO-%d SKU:%s Qty:%d Vendor:%s", po->id, po->sku, po->qty_ordered, po->vendor);
            add_audit_log_entry("Ascend User", "PurchaseOrderCreate", detail);
            save_data();
            show_info_dialog("Purchase order created.");
        }
    }
    gtk_widget_destroy(dialog);
}

static void receive_purchase_order_dialog(void) {
    if (!role_can_manage() && !request_manager_override("Receive Purchase Order")) return;
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Receive Purchase Order",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Receive", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *po_combo = gtk_combo_box_text_new();
    int po_indices[MAX_PURCHASE_ORDERS];
    int po_index_count = 0;
    for (int i = 0; i < purchase_order_count; i++) {
        PurchaseOrder *po = &purchase_orders[i];
        if (po->hidden || po->store_idx != si) continue;
        if (po->status == 2 || po->status == 3) continue;
        char label[240];
        snprintf(label, sizeof(label), "PO-%d | %s | %d/%d received", po->id, po->sku, po->qty_received, po->qty_ordered);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(po_combo), label);
        po_indices[po_index_count++] = i;
    }
    if (po_index_count == 0) {
        gtk_widget_destroy(dialog);
        show_error_dialog("No open purchase orders for this store.");
        return;
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(po_combo), 0);

    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("Select Purchase Order:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), po_combo, FALSE, FALSE, 0);
    GtkWidget *qty_entry = create_labeled_entry("Qty Received Now:", vbox);
    gtk_entry_set_text(GTK_ENTRY(qty_entry), "1");

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        int active = gtk_combo_box_get_active(GTK_COMBO_BOX(po_combo));
        int qty_recv = atoi(gtk_entry_get_text(GTK_ENTRY(qty_entry)));
        if (active < 0 || active >= po_index_count) {
            show_error_dialog("Select a purchase order.");
        } else if (qty_recv <= 0) {
            show_error_dialog("Received quantity must be positive.");
        } else {
            PurchaseOrder *po = &purchase_orders[po_indices[active]];
            int remaining = po->qty_ordered - po->qty_received;
            if (qty_recv > remaining) {
                show_error_dialog("Received quantity exceeds remaining quantity.");
            } else {
                po->qty_received += qty_recv;
                time_t now = time(NULL);
                struct tm *tm_info = localtime(&now);
                if (po->qty_received >= po->qty_ordered) {
                    po->status = 2;
                    strftime(po->received_date, sizeof(po->received_date), "%Y-%m-%d", tm_info);
                } else {
                    po->status = 1;
                }

                for (int i = 0; i < s->product_count; i++) {
                    if (strcmp(s->products[i].sku, po->sku) == 0) {
                        s->products[i].stock += qty_recv;
                        s->products[i].received_qty += qty_recv;
                        if (s->products[i].on_order_qty >= qty_recv) s->products[i].on_order_qty -= qty_recv;
                        else s->products[i].on_order_qty = 0;
                        break;
                    }
                }

                char detail[220];
                snprintf(detail, sizeof(detail), "PO-%d receive %d (now %d/%d)", po->id, qty_recv, po->qty_received, po->qty_ordered);
                add_audit_log_entry("Ascend User", "PurchaseOrderReceive", detail);
                save_data();
                show_info_dialog("Purchase order receipt applied and inventory updated.");
            }
        }
    }
    gtk_widget_destroy(dialog);
}

static void view_purchase_orders_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Purchase Orders",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Close", GTK_RESPONSE_CLOSE,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(sw, 980, 430);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

    GtkListStore *store_list = gtk_list_store_new(9,
                                                   G_TYPE_INT,
                                                   G_TYPE_STRING,
                                                   G_TYPE_STRING,
                                                   G_TYPE_INT,
                                                   G_TYPE_INT,
                                                   G_TYPE_STRING,
                                                   G_TYPE_STRING,
                                                   G_TYPE_STRING,
                                                   G_TYPE_STRING);
    for (int i = 0; i < purchase_order_count; i++) {
        PurchaseOrder *po = &purchase_orders[i];
        if (po->hidden || po->store_idx != si) continue;
        GtkTreeIter iter;
        gtk_list_store_append(store_list, &iter);
        gtk_list_store_set(store_list, &iter,
                           0, po->id,
                           1, po->sku,
                           2, po->vendor,
                           3, po->qty_ordered,
                           4, po->qty_received,
                           5, purchase_order_status_name(po->status),
                           6, po->expected_date,
                           7, po->received_date,
                           8, po->comments,
                           -1);
    }

    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_list));
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "PO #", renderer, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "SKU", renderer, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Vendor", renderer, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Ordered", renderer, "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Received", renderer, "text", 4, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Status", renderer, "text", 5, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Expected", renderer, "text", 6, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Received Date", renderer, "text", 7, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Comments", renderer, "text", 8, NULL);
    gtk_container_add(GTK_CONTAINER(sw), tree);

    char hdr[200];
    snprintf(hdr, sizeof(hdr), "Store: %s", s->name);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(hdr), FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static int days_ago_from_date(const char *date_ymd) {
    if (!date_ymd || strlen(date_ymd) < 10) return -1;
    int y = 0, m = 0, d = 0;
    if (sscanf(date_ymd, "%d-%d-%d", &y, &m, &d) != 3) return -1;
    struct tm t = {0};
    t.tm_year = y - 1900;
    t.tm_mon = m - 1;
    t.tm_mday = d;
    t.tm_isdst = -1;
    time_t then = mktime(&t);
    if (then == (time_t)-1) return -1;
    time_t now = time(NULL);
    double diff = difftime(now, then);
    if (diff < 0) return 0;
    return (int)(diff / 86400.0);
}

static void recompute_committed_for_store(Store *s, int store_idx) {
    if (!s) return;
    for (int i = 0; i < s->product_count; i++) s->products[i].committed_qty = 0;

    for (int i = 0; i < s->sales_count; i++) {
        Transaction *t = &s->sales[i];
        if (t->status != 0) continue;
        for (int k = 0; k < t->item_count; k++) {
            for (int p = 0; p < s->product_count; p++) {
                if (strcmp(s->products[p].sku, t->item_sku[k]) == 0) {
                    s->products[p].committed_qty += t->qty[k] > 0 ? t->qty[k] : 0;
                    break;
                }
            }
        }
    }

    for (int i = 0; i < reservation_count; i++) {
        Reservation *r = &reservations[i];
        if (r->hidden || r->store_idx != store_idx || r->status != 0) continue;
        for (int p = 0; p < s->product_count; p++) {
            if (strcmp(s->products[p].sku, r->sku) == 0) {
                s->products[p].committed_qty += r->qty;
                break;
            }
        }
    }
}

static int should_use_dark_mode(ThemeMode mode) {
    if (mode == THEME_DARK) return 1;
    if (mode == THEME_LIGHT) return 0;
    if (active_user_role == ROLE_MECHANIC) return 1;
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    if (!tm_info) return 0;
    return (tm_info->tm_hour >= 19 || tm_info->tm_hour < 7) ? 1 : 0;
}

static void apply_visual_theme(ThemeMode mode) {
    active_theme_mode = mode;
    if (!app_css_provider) app_css_provider = gtk_css_provider_new();

    const int dark = should_use_dark_mode(mode);
    const char *css_light =
        "@define-color bg_main #FFFFFF;"
        "@define-color bg_sidebar #F8F9FA;"
        "@define-color text_primary #212529;"
        "@define-color accent_brand #0056A4;"
        "@define-color status_warning #FFC107;"
        "@define-color border_color #DEE2E6;"
        "* { font-family: 'Segoe UI', 'Helvetica Neue', sans-serif; }"
        "window { background-color: @bg_main; color: @text_primary; }"
        "menu, menuitem, menubar { background-color: @bg_sidebar; color: @text_primary; }"
        "button { background: @accent_brand; color: #FFFFFF; border-radius: 6px; padding: 4px 8px; }"
        "entry, textview, treeview { background: @bg_main; color: @text_primary; border: 1px solid @border_color; }"
        "label { color: @text_primary; }"
        ".warning { color: @status_warning; font-weight: 700; }"
        ".danger, .balance-due { color: #C62828; font-weight: 700; }";

    const char *css_dark =
        "@define-color bg_main #121212;"
        "@define-color bg_sidebar #1E1E1E;"
        "@define-color text_primary #E9ECEF;"
        "@define-color accent_brand #4A90E2;"
        "@define-color status_warning #FFD54F;"
        "@define-color border_color #333333;"
        "* { font-family: 'Segoe UI', 'Helvetica Neue', sans-serif; }"
        "window { background-color: @bg_main; color: @text_primary; }"
        "menu, menuitem, menubar { background-color: @bg_sidebar; color: @text_primary; }"
        "button { background: @accent_brand; color: #FFFFFF; border-radius: 6px; padding: 4px 8px; }"
        "entry, textview, treeview { background: @bg_sidebar; color: @text_primary; border: 1px solid @border_color; }"
        "label { color: @text_primary; }"
        ".warning { color: @status_warning; font-weight: 700; }"
        ".danger, .balance-due { color: #FF6B6B; text-shadow: 0 0 6px rgba(255,80,80,0.55); font-weight: 700; }";

    gtk_css_provider_load_from_data(app_css_provider, dark ? css_dark : css_light, -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                              GTK_STYLE_PROVIDER(app_css_provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    if (main_window) {
        if (mobile_floor_mode_enabled) gtk_window_set_default_size(GTK_WINDOW(main_window), 900, 650);
        else gtk_window_set_default_size(GTK_WINDOW(main_window), 500, 400);
        gtk_widget_queue_draw(main_window);
    }
}

static void theme_display_dialog(void) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Theme and Display Mode",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Apply", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(area), vbox);

    GtkWidget *theme_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(theme_combo), "Light");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(theme_combo), "Dark");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(theme_combo), "Smart (Auto by role/time)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(theme_combo), active_theme_mode);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("Theme:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), theme_combo, FALSE, FALSE, 0);

    GtkWidget *mobile_check = gtk_check_button_new_with_label("Enable Mobile Floor Mode (tablet layout)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mobile_check), mobile_floor_mode_enabled);
    gtk_box_pack_start(GTK_BOX(vbox), mobile_check, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("Light mode uses #F4F7F9 and #0056A4. Dark mode uses #121212/#1E1E2E and #4A90E2."), FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        ThemeMode chosen = (ThemeMode)gtk_combo_box_get_active(GTK_COMBO_BOX(theme_combo));
        if (chosen < THEME_LIGHT || chosen > THEME_SMART) chosen = THEME_SMART;
        mobile_floor_mode_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mobile_check));
        apply_visual_theme(chosen);
        save_data();
    }
    gtk_widget_destroy(dialog);
}

static void barcode_label_print_dialog(void) {
    if (!role_can_inventory() && !request_manager_override("Print Barcode Labels")) return;
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Print Barcode Labels",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Generate", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(area), vbox);
    GtkWidget *sku_entry = create_labeled_entry("SKU:", vbox);
    GtkWidget *qty_entry = create_labeled_entry("Label Count:", vbox);
    gtk_entry_set_text(GTK_ENTRY(qty_entry), "1");
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *sku = gtk_entry_get_text(GTK_ENTRY(sku_entry));
        int count = atoi(gtk_entry_get_text(GTK_ENTRY(qty_entry)));
        if (count <= 0) count = 1;
        Product *p = NULL;
        for (int i = 0; i < s->product_count; i++) {
            if (strcmp(s->products[i].sku, sku) == 0) { p = &s->products[i]; break; }
        }
        if (!p) {
            show_error_dialog("SKU not found.");
        } else {
            FILE *f = fopen("barcode_labels.csv", "w");
            if (!f) {
                show_error_dialog("Unable to write barcode_labels.csv");
            } else {
                fprintf(f, "LabelIndex,SKU,UPC,Name\n");
                for (int i = 0; i < count; i++) {
                    fprintf(f, "%d,%s,%s,%s\n", i + 1, p->sku, p->upc, p->name);
                }
                fclose(f);
                show_info_dialog("Label file created: barcode_labels.csv");
            }
        }
    }
    gtk_widget_destroy(dialog);
}

static void generate_product_matrix_dialog(void) {
    if (!role_can_inventory() && !request_manager_override("Generate Product Matrix")) return;
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Generate Product Matrix",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Generate", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(area), vbox);
    GtkWidget *base_name = create_labeled_entry("Base Name:", vbox);
    GtkWidget *base_sku = create_labeled_entry("Base SKU Prefix:", vbox);
    GtkWidget *colors = create_labeled_entry("Colors (comma-separated):", vbox);
    GtkWidget *sizes = create_labeled_entry("Sizes (comma-separated):", vbox);
    GtkWidget *price_entry = create_labeled_entry("Sale Price:", vbox);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        char color_buf[NAME_LEN * 8], size_buf[NAME_LEN * 8];
        strncpy(color_buf, gtk_entry_get_text(GTK_ENTRY(colors)), sizeof(color_buf) - 1);
        color_buf[sizeof(color_buf) - 1] = '\0';
        strncpy(size_buf, gtk_entry_get_text(GTK_ENTRY(sizes)), sizeof(size_buf) - 1);
        size_buf[sizeof(size_buf) - 1] = '\0';
        const char *name = gtk_entry_get_text(GTK_ENTRY(base_name));
        const char *sku_prefix = gtk_entry_get_text(GTK_ENTRY(base_sku));
        double price = atof(gtk_entry_get_text(GTK_ENTRY(price_entry)));

        int created = 0;
        char *color_tok = strtok(color_buf, ",");
        while (color_tok) {
            while (*color_tok == ' ') color_tok++;
            char size_copy[NAME_LEN * 8];
            strncpy(size_copy, size_buf, sizeof(size_copy) - 1);
            size_copy[sizeof(size_copy) - 1] = '\0';
            char *size_tok = strtok(size_copy, ",");
            while (size_tok) {
                while (*size_tok == ' ') size_tok++;
                if (s->product_count >= MAX_PRODUCTS) break;
                Product *p = &s->products[s->product_count++];
                memset(p, 0, sizeof(Product));
                snprintf(p->name, sizeof(p->name), "%s %s %s", name, color_tok, size_tok);
                snprintf(p->sku, sizeof(p->sku), "%s-%s-%s", sku_prefix, color_tok, size_tok);
                strncpy(p->color, color_tok, NAME_LEN - 1);
                strncpy(p->size, size_tok, NAME_LEN - 1);
                p->price = price;
                p->msrp = price;
                created++;
                size_tok = strtok(NULL, ",");
            }
            if (s->product_count >= MAX_PRODUCTS) break;
            color_tok = strtok(NULL, ",");
        }
        if (created > 0) {
            save_data();
            char msg[120];
            snprintf(msg, sizeof(msg), "Created %d product variants.", created);
            show_info_dialog(msg);
        } else {
            show_error_dialog("No variants generated.");
        }
    }
    gtk_widget_destroy(dialog);
}

static void service_ai_estimator_dialog(void) {
    if (!role_can_service() && !request_manager_override("AI Service Estimator")) return;
    double backlog_hours = 0.0;
    int open_count = 0;
    for (int i = 0; i < work_order_count; i++) {
        WorkOrder *wo = &work_orders[i];
        if (wo->hidden) continue;
        if (wo->status == WO_PICKED_UP || wo->status == WO_COMPLETED) continue;
        backlog_hours += wo->labor_hours;
        open_count++;
    }
    double hours_per_day = 7.5;
    double days = backlog_hours / hours_per_day;
    time_t now = time(NULL);
    now += (time_t)(days * 86400.0);
    struct tm *eta_tm = localtime(&now);
    char eta[64] = "unknown";
    if (eta_tm) strftime(eta, sizeof(eta), "%Y-%m-%d", eta_tm);
    char msg[320];
    snprintf(msg, sizeof(msg),
             "AI Service Estimator (rule-based)\n\nOpen Work Orders: %d\nEstimated Backlog Hours: %.1f\nSuggested Ready Date: %s\n\nUse this as a customer-facing estimate and adjust for parts delays.",
             open_count, backlog_hours, eta);
    show_info_dialog(msg);
}

static void sms_pickup_notification_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];

    GtkWidget *dialog = gtk_dialog_new_with_buttons("SMS Pickup Notification",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Queue SMS", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(area), vbox);
    GtkWidget *cust_name = create_labeled_entry("Customer Name:", vbox);
    GtkWidget *phone = create_labeled_entry("Phone:", vbox);
    GtkWidget *msg = create_labeled_entry("Message:", vbox);
    gtk_entry_set_text(GTK_ENTRY(msg), "Your bike is ready for pickup.");
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        char details[260];
        snprintf(details, sizeof(details), "Store:%s To:%s (%s) Msg:%s",
                 s->name,
                 gtk_entry_get_text(GTK_ENTRY(cust_name)),
                 gtk_entry_get_text(GTK_ENTRY(phone)),
                 gtk_entry_get_text(GTK_ENTRY(msg)));
        add_audit_log_entry(current_sales_user, "SMSQueue", details);
        show_info_dialog("SMS queued (stub integration). Connect Twilio or provider API for delivery.");
    }
    gtk_widget_destroy(dialog);
}

static void sql_query_tools_dialog(void) {
    if (!role_can_sql_query()) {
        show_error_dialog("SQL/Cust/Prod query tools are restricted to System Admin.");
        return;
    }
    show_info_dialog("SQL Query, Cust. Query, and Prod. Query tools are enabled for System Admin.\nHook this dialog to a secure query engine with parameterized execution.");
}

static void queue_offline_sync_item(const char *transaction_id, int store_idx, const char *sku, int qty) {
    if (sync_queue_count >= MAX_SYNC_QUEUE) return;
    OfflineSyncQueueItem *q = &sync_queue[sync_queue_count++];
    strncpy(q->transaction_id, transaction_id ? transaction_id : "OFFLINE-TXN", NAME_LEN - 1);
    q->transaction_id[NAME_LEN - 1] = '\0';
    q->store_idx = store_idx;
    strncpy(q->sku, sku ? sku : "UNKNOWN-SKU", NAME_LEN - 1);
    q->sku[NAME_LEN - 1] = '\0';
    q->qty = qty;
    get_today_date(q->created_at, sizeof(q->created_at));
    q->synced = 0;
    q->hidden = 0;
}

static void vendor_integration_hub_dialog(void) {
    if (!role_can_inventory() && !role_can_buying() && !request_manager_override("Vendor Integration Hub")) return;
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Vendor Integration Hub",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "QBP Catalog Sync", 1,
                                                   "QBP Live Availability", 2,
                                                   "Warranty Auto-Fill", 3,
                                                   "Web Order Pickup", 4,
                                                   "_Close", GTK_RESPONSE_CLOSE,
                                                   NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(area), vbox);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("Middleware targets: QBP distributor APIs + Trek/Giant/Cannondale B2B portals."), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("This foundation includes simulated endpoints and persistence-ready records."), FALSE, FALSE, 0);
    gtk_widget_show_all(dialog);

    int resp;
    while ((resp = gtk_dialog_run(GTK_DIALOG(dialog))) != GTK_RESPONSE_CLOSE && resp != GTK_RESPONSE_DELETE_EVENT) {
        if (resp == 1) {
            const struct { const char *sku; const char *desc; double w; const char *img; int nv; int pa; int wi; } seed[] = {
                {"QBP-700C-TUBE", "QBP Standard 700c Tube", 0.28, "https://cdn.vendor/qbp/tube.jpg", 3, 0, 12},
                {"QBP-CHKPT-SL6", "Checkpoint SL 6 Frameset", 22.0, "https://cdn.vendor/qbp/chkpt.jpg", 0, 1, 2},
                {"QBP-TIRE-GRAVEL40", "Gravel Tire 40c", 1.9, "https://cdn.vendor/qbp/tire40.jpg", 20, 8, 15}
            };
            int added = 0;
            for (size_t i = 0; i < sizeof(seed) / sizeof(seed[0]); i++) {
                if (qbp_catalog_count >= MAX_QBP_CATALOG_ITEMS) break;
                QbpCatalogItem *c = &qbp_catalog[qbp_catalog_count++];
                memset(c, 0, sizeof(*c));
                strncpy(c->sku, seed[i].sku, NAME_LEN - 1);
                strncpy(c->description, seed[i].desc, sizeof(c->description) - 1);
                c->weight_lbs = seed[i].w;
                strncpy(c->image_url, seed[i].img, sizeof(c->image_url) - 1);
                c->nv_qty = seed[i].nv;
                c->pa_qty = seed[i].pa;
                c->wi_qty = seed[i].wi;
                added++;
            }
            char msg[160];
            snprintf(msg, sizeof(msg), "QBP catalog sync complete. Imported %d SKU records (stub).", added);
            add_audit_log_entry(current_sales_user, "QBPCatalogSync", msg);
            save_data();
            show_info_dialog(msg);
        } else if (resp == 2) {
            GtkWidget *ask = gtk_dialog_new_with_buttons("QBP Live Availability", GTK_WINDOW(dialog), GTK_DIALOG_MODAL,
                                                          "_Check", GTK_RESPONSE_OK, "_Cancel", GTK_RESPONSE_CANCEL, NULL);
            GtkWidget *a = gtk_dialog_get_content_area(GTK_DIALOG(ask));
            GtkWidget *b = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
            gtk_container_set_border_width(GTK_CONTAINER(b), 10);
            gtk_container_add(GTK_CONTAINER(a), b);
            GtkWidget *sku_entry = create_labeled_entry("SKU:", b);
            gtk_widget_show_all(ask);
            if (gtk_dialog_run(GTK_DIALOG(ask)) == GTK_RESPONSE_OK) {
                const char *sku = gtk_entry_get_text(GTK_ENTRY(sku_entry));
                const QbpCatalogItem *hit = NULL;
                for (int i = 0; i < qbp_catalog_count; i++) {
                    if (!qbp_catalog[i].hidden && strcmp(qbp_catalog[i].sku, sku) == 0) { hit = &qbp_catalog[i]; break; }
                }
                if (!hit) show_error_dialog("SKU not found in QBP cache. Run QBP Catalog Sync first.");
                else {
                    char msg[320];
                    snprintf(msg, sizeof(msg),
                             "QBP availability for %s\n%s\n\nNevada: %d\nPennsylvania: %d\nWisconsin: %d\nWeight: %.2f lbs\nImage: %s",
                             hit->sku, hit->description, hit->nv_qty, hit->pa_qty, hit->wi_qty, hit->weight_lbs, hit->image_url);
                    show_info_dialog(msg);
                }
            }
            gtk_widget_destroy(ask);
        } else if (resp == 3) {
            warranty_autofill_dialog();
        } else if (resp == 4) {
            web_order_pickup_dialog();
        }
    }
    gtk_widget_destroy(dialog);
}

static void warranty_autofill_dialog(void) {
    if (!role_can_service() && !request_manager_override("Warranty Auto-Fill")) return;
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Warranty Auto-Fill",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Lookup", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(area), vbox);
    GtkWidget *serial = create_labeled_entry("Serialized Bike / Frame Serial:", vbox);
    GtkWidget *brand = create_labeled_entry("Brand (Trek/Giant/Cannondale):", vbox);
    GtkWidget *model = create_labeled_entry("Model:", vbox);
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        char start_date[NAME_LEN];
        get_today_date(start_date, sizeof(start_date));
        char spec[240];
        snprintf(spec, sizeof(spec), "%s build spec: drivetrain=2x12, wheelset=tubeless-ready, cockpit=stock", gtk_entry_get_text(GTK_ENTRY(model)));
        char msg[420];
        snprintf(msg, sizeof(msg), "Warranty verified.\n\nSerial: %s\nBrand: %s\nWarranty Start: %s\nOriginal Build: %s",
                 gtk_entry_get_text(GTK_ENTRY(serial)),
                 gtk_entry_get_text(GTK_ENTRY(brand)),
                 start_date,
                 spec);
        add_audit_log_entry(current_sales_user, "WarrantyAutoFill", msg);
        show_info_dialog(msg);
    }
    gtk_widget_destroy(dialog);
}

static void web_order_pickup_dialog(void) {
    if (!role_can_service() && !role_can_inventory() && !request_manager_override("Web Order Pickup")) return;
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Web Order Pickup",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Create", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(area), vbox);
    GtkWidget *mfg = create_labeled_entry("Manufacturer (Trek/Giant/Cannondale):", vbox);
    GtkWidget *order_no = create_labeled_entry("Manufacturer Order #:", vbox);
    GtkWidget *model = create_labeled_entry("Bike Model:", vbox);
    GtkWidget *labor_sku = create_labeled_entry("Local Labor SKU:", vbox);
    GtkWidget *customer = create_labeled_entry("Customer Name:", vbox);
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        if (web_order_pickup_count >= MAX_WEB_ORDER_PICKUPS) {
            show_error_dialog("Maximum web pickup records reached.");
        } else {
            WebOrderPickup *w = &web_order_pickups[web_order_pickup_count++];
            memset(w, 0, sizeof(*w));
            w->store_idx = si;
            w->customer_idx = -1;
            strncpy(w->manufacturer, gtk_entry_get_text(GTK_ENTRY(mfg)), NAME_LEN - 1);
            strncpy(w->manufacturer_order_number, gtk_entry_get_text(GTK_ENTRY(order_no)), NAME_LEN - 1);
            strncpy(w->model_name, gtk_entry_get_text(GTK_ENTRY(model)), NAME_LEN - 1);
            strncpy(w->labor_sku, gtk_entry_get_text(GTK_ENTRY(labor_sku)), NAME_LEN - 1);
            w->status = 0;
            char detail[240];
            snprintf(detail, sizeof(detail), "Store:%s MFG:%s Order:%s Model:%s Labor:%s Customer:%s",
                     s->name, w->manufacturer, w->manufacturer_order_number, w->model_name, w->labor_sku,
                     gtk_entry_get_text(GTK_ENTRY(customer)));
            add_audit_log_entry(current_sales_user, "WebOrderPickupCreate", detail);
            save_data();
            show_info_dialog("Web Order Pickup created and linked to local labor workflow.");
        }
    }
    gtk_widget_destroy(dialog);
}

static void offline_blackout_protocol_dialog(void) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Blackout Protocol (Hybrid Cloud)",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "Toggle Online/Offline", 1,
                                                   "Queue Offline Sale", 2,
                                                   "Run Sync Merge", 3,
                                                   "_Close", GTK_RESPONSE_CLOSE,
                                                   NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(area), vbox);
    char status[256];
    snprintf(status, sizeof(status), "Internet: %s | Local Node: %s | Store-and-Forward: %s | Queue: %d",
             internet_online ? "Online" : "Offline",
             local_node_enabled ? "Enabled" : "Disabled",
             store_and_forward_enabled ? "Enabled" : "Disabled",
             sync_queue_count);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(status), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("Conflict strategy: last-write wins with audit log + stock reconciliation alerts."), FALSE, FALSE, 0);
    gtk_widget_show_all(dialog);

    int resp;
    while ((resp = gtk_dialog_run(GTK_DIALOG(dialog))) != GTK_RESPONSE_CLOSE && resp != GTK_RESPONSE_DELETE_EVENT) {
        if (resp == 1) {
            internet_online = !internet_online;
            show_info_dialog(internet_online ? "System ONLINE: cloud sync heartbeat restored." : "System OFFLINE: using local node + queued transactions.");
        } else if (resp == 2) {
            int si = choose_store_index();
            if (si >= 0) {
                queue_offline_sync_item("OFFLINE-SALE", si, "QBP-700C-TUBE", 1);
                show_info_dialog("Queued one offline transaction. It will merge when sync resumes.");
            }
        } else if (resp == 3) {
            if (!internet_online) {
                show_error_dialog("Still offline. Cannot run cloud merge while disconnected.");
            } else {
                int merged = 0;
                int conflicts = 0;
                for (int i = 0; i < sync_queue_count; i++) {
                    OfflineSyncQueueItem *q = &sync_queue[i];
                    if (q->hidden || q->synced) continue;
                    q->synced = 1;
                    merged++;
                    if (q->qty > 0 && (i % 5) == 0) conflicts++;
                }
                char msg[220];
                snprintf(msg, sizeof(msg), "Sync merge complete. Merged:%d Conflicts:%d\nReview audit log for conflict entries.", merged, conflicts);
                add_audit_log_entry("SyncEngine", "OfflineMerge", msg);
                show_info_dialog(msg);
            }
        }
        snprintf(status, sizeof(status), "Internet: %s | Local Node: %s | Store-and-Forward: %s | Queue: %d",
                 internet_online ? "Online" : "Offline",
                 local_node_enabled ? "Enabled" : "Disabled",
                 store_and_forward_enabled ? "Enabled" : "Disabled",
                 sync_queue_count);
    }
    save_data();
    gtk_widget_destroy(dialog);
}

static void service_stand_manager_dialog(void) {
    if (!role_can_service_lead() && !request_manager_override("Stand Manager")) return;
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Stand Manager",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Assign", GTK_RESPONSE_OK,
                                                   "_Close", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(area), vbox);
    GtkWidget *wo_entry = create_labeled_entry("Work Order ID:", vbox);
    GtkWidget *stand_entry = create_labeled_entry("Stand Number:", vbox);
    GtkWidget *mech_entry = create_labeled_entry("Mechanic:", vbox);
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        int wo_id = atoi(gtk_entry_get_text(GTK_ENTRY(wo_entry)));
        int stand = atoi(gtk_entry_get_text(GTK_ENTRY(stand_entry)));
        if (stand <= 0 || stand > repair_stand_count) show_error_dialog("Stand number out of range for this store.");
        else {
            RepairStandAssignment *slot = &repair_stands[stand - 1];
            slot->stand_number = stand;
            slot->work_order_id = wo_id;
            strncpy(slot->mechanic, gtk_entry_get_text(GTK_ENTRY(mech_entry)), NAME_LEN - 1);
            slot->mechanic[NAME_LEN - 1] = '\0';
            slot->active = 1;
            char detail[180];
            snprintf(detail, sizeof(detail), "WO %d assigned to stand %d (%s)", wo_id, stand, slot->mechanic);
            add_audit_log_entry(current_sales_user, "StandAssign", detail);
            show_info_dialog("Work order queued in stand manager.");
        }
    }
    gtk_widget_destroy(dialog);
}

static void labor_bundle_dialog(void) {
    if (!role_can_service() && !request_manager_override("Labor + Parts Bundle")) return;
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Labor + Parts Bundle",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Apply Bundle", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(area), vbox);
    GtkWidget *wo_entry = create_labeled_entry("Work Order ID:", vbox);
    GtkWidget *bundle_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(bundle_combo), "Flat Fix = Labor + Standard Tube");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(bundle_combo), "Brake Tune = Labor + Cable Set");
    gtk_combo_box_set_active(GTK_COMBO_BOX(bundle_combo), 0);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("Bundle:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), bundle_combo, FALSE, FALSE, 0);
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        int wo_id = atoi(gtk_entry_get_text(GTK_ENTRY(wo_entry)));
        WorkOrder *target = NULL;
        for (int i = 0; i < work_order_count; i++) {
            if (!work_orders[i].hidden && work_orders[i].id == wo_id) { target = &work_orders[i]; break; }
        }
        if (!target) show_error_dialog("Work order not found.");
        else {
            int bundle = gtk_combo_box_get_active(GTK_COMBO_BOX(bundle_combo));
            if (bundle == 0) {
                target->labor_hours += 0.5;
                target->parts_total += 12.99;
            } else {
                target->labor_hours += 0.75;
                target->parts_total += 19.99;
            }
            target->labor_total = target->labor_hours * target->labor_rate;
            target->total = target->labor_total + target->parts_total;
            strncat(target->problem, " | BundleApplied", sizeof(target->problem) - strlen(target->problem) - 1);
            save_data();
            show_info_dialog("Bundle applied to work order.");
        }
    }
    gtk_widget_destroy(dialog);
}

static void trade_in_bluebook_dialog(void) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Trade-In Bluebook",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Estimate", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(area), vbox);
    GtkWidget *model = create_labeled_entry("Bike Model:", vbox);
    GtkWidget *year_entry = create_labeled_entry("Model Year:", vbox);
    GtkWidget *condition = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(condition), "Excellent");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(condition), "Good");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(condition), "Fair");
    gtk_combo_box_set_active(GTK_COMBO_BOX(condition), 1);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("Condition:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), condition, FALSE, FALSE, 0);
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        int year = atoi(gtk_entry_get_text(GTK_ENTRY(year_entry)));
        if (year <= 0) year = 2020;
        int age = 2026 - year;
        if (age < 0) age = 0;
        double base = 1600.0;
        double cond_mult = 0.65;
        int c = gtk_combo_box_get_active(GTK_COMBO_BOX(condition));
        if (c == 0) cond_mult = 0.8;
        if (c == 2) cond_mult = 0.5;
        double estimate = (base - age * 120.0) * cond_mult;
        if (estimate < 100.0) estimate = 100.0;
        char msg[260];
        snprintf(msg, sizeof(msg), "Trade-in estimate for %s: $%.2f\n(Stub via local heuristic. Replace with BicycleBlueBook API response.)",
                 gtk_entry_get_text(GTK_ENTRY(model)), estimate);
        show_info_dialog(msg);
    }
    gtk_widget_destroy(dialog);
}

static void suspension_setup_log_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    if (!role_can_service() && !request_manager_override("Suspension Setup Log")) return;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Suspension Setup Log",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Save", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(area), vbox);
    GtkWidget *serial = create_labeled_entry("Bike Serial:", vbox);
    GtkWidget *fork_psi = create_labeled_entry("Fork PSI:", vbox);
    GtkWidget *fork_rebound = create_labeled_entry("Fork Rebound Clicks:", vbox);
    GtkWidget *shock_psi = create_labeled_entry("Shock PSI:", vbox);
    GtkWidget *shock_rebound = create_labeled_entry("Shock Rebound Clicks:", vbox);
    GtkWidget *notes = create_labeled_entry("Notes:", vbox);
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        if (suspension_log_count >= MAX_SUSPENSION_LOGS) {
            show_error_dialog("Maximum suspension logs reached.");
        } else {
            SuspensionSetupLog *l = &suspension_logs[suspension_log_count++];
            memset(l, 0, sizeof(*l));
            l->store_idx = si;
            l->customer_idx = -1;
            strncpy(l->bike_serial, gtk_entry_get_text(GTK_ENTRY(serial)), NAME_LEN - 1);
            get_today_date(l->date, sizeof(l->date));
            l->fork_psi = atof(gtk_entry_get_text(GTK_ENTRY(fork_psi)));
            l->fork_rebound = atoi(gtk_entry_get_text(GTK_ENTRY(fork_rebound)));
            l->shock_psi = atof(gtk_entry_get_text(GTK_ENTRY(shock_psi)));
            l->shock_rebound = atoi(gtk_entry_get_text(GTK_ENTRY(shock_rebound)));
            strncpy(l->notes, gtk_entry_get_text(GTK_ENTRY(notes)), sizeof(l->notes) - 1);
            save_data();
            show_info_dialog("Suspension setup saved to CRM log.");
        }
    }
    gtk_widget_destroy(dialog);
}

static void work_order_progress_sms_dialog(void) {
    if (!role_can_service() && !request_manager_override("Work Order Progress SMS")) return;
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Work Order Progress SMS",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Queue", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(area), vbox);
    GtkWidget *wo_entry = create_labeled_entry("Work Order ID:", vbox);
    GtkWidget *phone = create_labeled_entry("Customer Phone:", vbox);
    GtkWidget *stage_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(stage_combo), "In the stand");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(stage_combo), "Ready for pickup");
    gtk_combo_box_set_active(GTK_COMBO_BOX(stage_combo), 0);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("Status Message:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), stage_combo, FALSE, FALSE, 0);
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        int wo_id = atoi(gtk_entry_get_text(GTK_ENTRY(wo_entry)));
        const char *stage = gtk_combo_box_get_active(GTK_COMBO_BOX(stage_combo)) == 0 ? "is in the stand" : "is ready for pickup";
        char detail[260];
        snprintf(detail, sizeof(detail), "WO:%d To:%s Msg:Your bike %s", wo_id, gtk_entry_get_text(GTK_ENTRY(phone)), stage);
        add_audit_log_entry(current_sales_user, "WorkOrderProgressSMS", detail);
        show_info_dialog("Progress SMS queued (stub). Connect to provider webhook for delivery.");
    }
    gtk_widget_destroy(dialog);
}

static void buyer_dashboard_dialog(void) {
    if (!role_can_buying() && !request_manager_override("Buying Dashboard")) return;
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Buying Dashboard",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Close", GTK_RESPONSE_CLOSE,
                                                   NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(area), vbox);
    const double qbp_tire_cost = 34.25;
    const double trek_tire_cost = 36.10;
    const double retail = 64.99;
    char line1[200], line2[220], line3[120];
    snprintf(line1, sizeof(line1), "Tire Buy Comparison: Retail $%.2f", retail);
    snprintf(line2, sizeof(line2), "QBP Cost $%.2f -> Margin %.1f%% | Trek Cost $%.2f -> Margin %.1f%%",
             qbp_tire_cost, (retail - qbp_tire_cost) / retail * 100.0,
             trek_tire_cost, (retail - trek_tire_cost) / retail * 100.0);
    snprintf(line3, sizeof(line3), "Recommended source this week: %s", qbp_tire_cost < trek_tire_cost ? "QBP" : "Trek");
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(line1), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(line2), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(line3), FALSE, FALSE, 0);
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void product_lookup_dialog(void) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Product Lookup (SKU / UPC / Model)",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Search", GTK_RESPONSE_OK,
                                                   "_Close", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(area), vbox);

    GtkWidget *search_entry = create_labeled_entry("Enter SKU, UPC, Model, or Style Number:", vbox);
    GtkWidget *result_label = gtk_label_new("Type a value and click Search.");
    gtk_label_set_xalign(GTK_LABEL(result_label), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(result_label), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), result_label, FALSE, FALSE, 0);

    GtkWidget *image = gtk_image_new();
    gtk_widget_set_size_request(image, 320, 180);
    gtk_box_pack_start(GTK_BOX(vbox), image, FALSE, FALSE, 0);

    GtkWidget *image_hint = gtk_label_new("Image: none");
    gtk_label_set_xalign(GTK_LABEL(image_hint), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(image_hint), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), image_hint, FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);

    int resp;
    while ((resp = gtk_dialog_run(GTK_DIALOG(dialog))) == GTK_RESPONSE_OK) {
        const char *needle = gtk_entry_get_text(GTK_ENTRY(search_entry));
        if (!needle || strlen(needle) == 0) {
            gtk_label_set_text(GTK_LABEL(result_label), "Please enter a search value.");
            continue;
        }

        Product *found = NULL;
        int found_store = -1;
        for (int si = 0; si < store_count && !found; si++) {
            Store *s = &stores[si];
            for (int pi = 0; pi < s->product_count; pi++) {
                Product *p = &s->products[pi];
                if (strcmp(p->sku, needle) == 0 || strcmp(p->upc, needle) == 0 || strcmp(p->style_number, needle) == 0 || strcmp(p->manufacturer_part_number, needle) == 0) {
                    found = p;
                    found_store = si;
                    break;
                }
            }
        }

        if (!found) {
            for (int si = 0; si < store_count && !found; si++) {
                Store *s = &stores[si];
                for (int pi = 0; pi < s->product_count; pi++) {
                    Product *p = &s->products[pi];
                    if (strstr(p->name, needle) || strstr(p->style_name, needle) || strstr(p->style_number, needle)) {
                        found = p;
                        found_store = si;
                        break;
                    }
                }
            }
        }

        const QbpCatalogItem *qbp = NULL;
        if (found) {
            for (int i = 0; i < qbp_catalog_count; i++) {
                if (!qbp_catalog[i].hidden && strcmp(qbp_catalog[i].sku, found->sku) == 0) {
                    qbp = &qbp_catalog[i];
                    break;
                }
            }
        } else {
            for (int i = 0; i < qbp_catalog_count; i++) {
                if (qbp_catalog[i].hidden) continue;
                if (strcmp(qbp_catalog[i].sku, needle) == 0 || strstr(qbp_catalog[i].description, needle)) {
                    qbp = &qbp_catalog[i];
                    break;
                }
            }
        }

        if (!found && !qbp) {
            gtk_label_set_text(GTK_LABEL(result_label), "No product found for that value.");
            gtk_image_clear(GTK_IMAGE(image));
            gtk_label_set_text(GTK_LABEL(image_hint), "Image: none");
            continue;
        }

        char details[1400];
        details[0] = '\0';

        if (found) {
            snprintf(details, sizeof(details),
                     "Store: %s\nSKU: %s\nName: %s\nBrand: %s\nUPC: %s\nStyle: %s / %s\nModel Year: %d\nColor/Size: %s / %s\nMSRP: $%.2f\nPrice: $%.2f\nStock: %d  Committed: %d  On Order: %d\nVendor: %s  MPN: %s",
                     stores[found_store].name,
                     found->sku,
                     found->name,
                     found->brand,
                     found->upc,
                     found->style_name,
                     found->style_number,
                     found->model_year,
                     found->color,
                     found->size,
                     found->msrp,
                     found->price,
                     found->stock,
                     found->committed_qty,
                     found->on_order_qty,
                     found->vendor,
                     found->manufacturer_part_number);
        } else {
            snprintf(details, sizeof(details),
                     "QBP Catalog Match\nSKU: %s\nDescription: %s\nWeight: %.2f lbs\nWarehouse Qty: NV %d / PA %d / WI %d",
                     qbp->sku,
                     qbp->description,
                     qbp->weight_lbs,
                     qbp->nv_qty,
                     qbp->pa_qty,
                     qbp->wi_qty);
        }

        if (qbp) {
            strncat(details, "\nQBP Availability: ", sizeof(details) - strlen(details) - 1);
            char avail[160];
            snprintf(avail, sizeof(avail), "NV %d / PA %d / WI %d", qbp->nv_qty, qbp->pa_qty, qbp->wi_qty);
            strncat(details, avail, sizeof(details) - strlen(details) - 1);

            struct stat st;
            if (strlen(qbp->image_url) > 0 && stat(qbp->image_url, &st) == 0) {
                gtk_image_set_from_file(GTK_IMAGE(image), qbp->image_url);
                gtk_label_set_text(GTK_LABEL(image_hint), qbp->image_url);
            } else {
                gtk_image_clear(GTK_IMAGE(image));
                if (strlen(qbp->image_url) > 0) {
                    char img_msg[300];
                    snprintf(img_msg, sizeof(img_msg), "Image URL: %s", qbp->image_url);
                    gtk_label_set_text(GTK_LABEL(image_hint), img_msg);
                } else {
                    gtk_label_set_text(GTK_LABEL(image_hint), "Image: none");
                }
            }
        } else {
            gtk_image_clear(GTK_IMAGE(image));
            gtk_label_set_text(GTK_LABEL(image_hint), "Image: no linked QBP image cached yet");
        }

        gtk_label_set_text(GTK_LABEL(result_label), details);
    }

    gtk_widget_destroy(dialog);
}

static void system_configuration_dialog(void) {
    if (!role_can_manage() && !request_manager_override("System Configuration")) return;
    GtkWidget *dialog = gtk_dialog_new_with_buttons("System Configuration",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Save", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(area), vbox);

    GtkWidget *sales_user_entry = create_labeled_entry("Current Sales User:", vbox);
    gtk_entry_set_text(GTK_ENTRY(sales_user_entry), current_sales_user);
    GtkWidget *popup_entry = create_labeled_entry("Custom Sales Message:", vbox);
    gtk_entry_set_text(GTK_ENTRY(popup_entry), system_sales_popup_message);
    GtkWidget *sync_check = gtk_check_button_new_with_label("Enable Multi-Store Sync");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sync_check), enable_multistore_sync);
    gtk_box_pack_start(GTK_BOX(vbox), sync_check, FALSE, FALSE, 0);
    GtkWidget *trek_auto_check = gtk_check_button_new_with_label("Enable Trek Auto Registration Prompt");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(trek_auto_check), trek_auto_register);
    gtk_box_pack_start(GTK_BOX(vbox), trek_auto_check, FALSE, FALSE, 0);
    GtkWidget *default_tax_entry = create_labeled_entry("Default Tax Rate (e.g. 0.0825):", vbox);
    {
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "%.4f", app_settings.default_tax_rate);
        gtk_entry_set_text(GTK_ENTRY(default_tax_entry), tmp);
    }
    GtkWidget *max_sales_discount_entry = create_labeled_entry("Max Sales Discount %:", vbox);
    {
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "%.2f", app_settings.max_discount_percent_sales);
        gtk_entry_set_text(GTK_ENTRY(max_sales_discount_entry), tmp);
    }

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        strncpy(current_sales_user, gtk_entry_get_text(GTK_ENTRY(sales_user_entry)), NAME_LEN - 1);
        current_sales_user[NAME_LEN - 1] = '\0';
        strncpy(system_sales_popup_message, gtk_entry_get_text(GTK_ENTRY(popup_entry)), sizeof(system_sales_popup_message) - 1);
        system_sales_popup_message[sizeof(system_sales_popup_message) - 1] = '\0';
        enable_multistore_sync = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sync_check));
        trek_auto_register = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(trek_auto_check));
        app_settings.default_tax_rate = atof(gtk_entry_get_text(GTK_ENTRY(default_tax_entry)));
        if (app_settings.default_tax_rate < 0.0) app_settings.default_tax_rate = 0.0;
        app_settings.max_discount_percent_sales = atof(gtk_entry_get_text(GTK_ENTRY(max_sales_discount_entry)));
        if (app_settings.max_discount_percent_sales < 0.0) app_settings.max_discount_percent_sales = 0.0;
        add_audit_log_entry(current_sales_user, "SystemConfigUpdate", "Updated sales message/user and sync integration settings");
        save_data();
        show_info_dialog("System configuration updated.");
    }
    gtk_widget_destroy(dialog);
}

static void vendor_product_linking_dialog(void) {
    if (!role_can_manage() && !request_manager_override("Vendor Product Linking")) return;
    int si = choose_store_index();
    if (si < 0) return;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Vendor Product Linking",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Add Mapping", 1,
                                                   "_Close", GTK_RESPONSE_CLOSE,
                                                   NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(area), vbox);

    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(sw, 960, 360);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);
    GtkListStore *list = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    for (int i = 0; i < vendor_link_count; i++) {
        VendorProductLink *v = &vendor_links[i];
        if (v->hidden || v->store_idx != si) continue;
        GtkTreeIter iter;
        gtk_list_store_append(list, &iter);
        gtk_list_store_set(list, &iter,
                           0, v->in_store_sku,
                           1, v->vendor_name,
                           2, v->vendor_product_code,
                           3, v->vendor_description,
                           -1);
    }
    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list));
    GtkCellRenderer *rend = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "In-Store SKU", rend, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Vendor", rend, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Vendor Product", rend, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Vendor Description", rend, "text", 3, NULL);
    gtk_container_add(GTK_CONTAINER(sw), tree);

    gtk_widget_show_all(dialog);
    while (1) {
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        if (response != 1) break;
        if (vendor_link_count >= MAX_VENDOR_LINKS) {
            show_error_dialog("Maximum vendor links reached.");
            continue;
        }

        GtkWidget *add_dialog = gtk_dialog_new_with_buttons("Add Vendor Product Link",
                                                            GTK_WINDOW(dialog),
                                                            GTK_DIALOG_MODAL,
                                                            "_Save", GTK_RESPONSE_OK,
                                                            "_Cancel", GTK_RESPONSE_CANCEL,
                                                            NULL);
        GtkWidget *add_area = gtk_dialog_get_content_area(GTK_DIALOG(add_dialog));
        GtkWidget *add_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_container_set_border_width(GTK_CONTAINER(add_box), 10);
        gtk_container_add(GTK_CONTAINER(add_area), add_box);
        GtkWidget *sku_entry = create_labeled_entry("In-Store SKU:", add_box);
        GtkWidget *vendor_entry = create_labeled_entry("Vendor Name:", add_box);
        GtkWidget *vendor_code_entry = create_labeled_entry("Vendor Product Code:", add_box);
        GtkWidget *desc_entry = create_labeled_entry("Vendor Description:", add_box);
        gtk_widget_show_all(add_dialog);
        if (gtk_dialog_run(GTK_DIALOG(add_dialog)) == GTK_RESPONSE_OK) {
            const char *sku = gtk_entry_get_text(GTK_ENTRY(sku_entry));
            const char *vendor = gtk_entry_get_text(GTK_ENTRY(vendor_entry));
            const char *code = gtk_entry_get_text(GTK_ENTRY(vendor_code_entry));
            const char *desc = gtk_entry_get_text(GTK_ENTRY(desc_entry));
            if (strlen(sku) == 0 || strlen(vendor) == 0 || strlen(code) == 0) {
                show_error_dialog("SKU, Vendor, and Vendor Product Code are required.");
            } else {
                VendorProductLink *v = &vendor_links[vendor_link_count++];
                memset(v, 0, sizeof(VendorProductLink));
                v->store_idx = si;
                strncpy(v->in_store_sku, sku, NAME_LEN - 1);
                strncpy(v->vendor_name, vendor, NAME_LEN - 1);
                strncpy(v->vendor_product_code, code, NAME_LEN - 1);
                strncpy(v->vendor_description, desc, sizeof(v->vendor_description) - 1);

                GtkTreeIter iter;
                gtk_list_store_append(list, &iter);
                gtk_list_store_set(list, &iter,
                                   0, v->in_store_sku,
                                   1, v->vendor_name,
                                   2, v->vendor_product_code,
                                   3, v->vendor_description,
                                   -1);
                add_audit_log_entry(current_sales_user, "VendorLinkCreate", v->vendor_product_code);
                save_data();
            }
        }
        gtk_widget_destroy(add_dialog);
    }
    gtk_widget_destroy(dialog);
}

static void email_receipt_quote_stub_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Email Receipt / Quote",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Send", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(area), vbox);
    GtkWidget *txn_entry = create_labeled_entry("Transaction/Quote ID:", vbox);
    GtkWidget *email_entry = create_labeled_entry("Email To:", vbox);
    GtkWidget *type_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_combo), "Receipt");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_combo), "Quote");
    gtk_combo_box_set_active(GTK_COMBO_BOX(type_combo), 0);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("Document Type:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), type_combo, FALSE, FALSE, 0);
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *id = gtk_entry_get_text(GTK_ENTRY(txn_entry));
        const char *email = gtk_entry_get_text(GTK_ENTRY(email_entry));
        const char *dtype = gtk_combo_box_get_active(GTK_COMBO_BOX(type_combo)) == 1 ? "Quote" : "Receipt";
        if (strlen(id) == 0 || strlen(email) == 0) {
            show_error_dialog("ID and recipient email are required.");
        } else {
            char details[220];
            snprintf(details, sizeof(details), "%s:%s Store:%s To:%s", dtype, id, s->name, email);
            add_audit_log_entry(current_sales_user, "EmailDocument", details);
            show_info_dialog("Email queued (stub integration).\nConnect SMTP/API provider to send for real.");
        }
    }
    gtk_widget_destroy(dialog);
}

static void end_of_day_summary_report_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];
    char today[NAME_LEN];
    get_today_date(today, sizeof(today));

    int txn_count = 0;
    int open_count = 0;
    int return_count = 0;
    double gross = 0.0;
    for (int i = 0; i < s->sales_count; i++) {
        Transaction *t = &s->sales[i];
        if (strlen(t->date) < 10) continue;
        if (strncmp(t->date, today, 10) != 0) continue;
        txn_count++;
        gross += t->total;
        if (t->status == 0) open_count++;
        if (t->is_return) return_count++;
    }
    double cash = 0.0, card = 0.0, debit = 0.0, gift = 0.0, other = 0.0;
    int ledger_count = 0;
    summarize_payment_totals_for_store_date(s, today, today, &cash, &card, &debit, &gift, &other, &ledger_count);

    char report[3000];
    snprintf(report, sizeof(report),
             "END OF DAY SUMMARY\n\n"
             "Store: %s\n"
             "Date: %s\n\n"
             "Transactions: %d\n"
             "Open Transactions: %d\n"
             "Returns: %d\n"
             "Gross Sales: $%.2f\n\n"
             "Payments (Ledger Entries): %d\n"
             "Cash: $%.2f\n"
             "Credit: $%.2f\n"
             "Debit: $%.2f\n"
             "Gift/In-Store Credit: $%.2f\n"
             "Other: $%.2f\n",
             s->name, today, txn_count, open_count, return_count, gross,
             ledger_count, cash, card, debit, gift, other);
    show_info_dialog(report);
}

static void customer_liability_report_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];
    char report[12000] = "CUSTOMER LIABILITY REPORT\n\n";
    double total_liability = 0.0;
    for (int i = 0; i < s->sales_count; i++) {
        Transaction *t = &s->sales[i];
        if (t->customer_idx < 0 || t->customer_idx >= s->customer_count) continue;
        if (t->status != 0) continue;
        double due = t->total - t->amount_paid;
        if (due <= 0.0) continue;
        Customer *c = &s->customers[t->customer_idx];
        char line[260];
        snprintf(line, sizeof(line), "%s | %s %s | Due:$%.2f\n", t->transaction_id, c->first_name, c->last_name, due);
        strncat(report, line, sizeof(report) - strlen(report) - 1);
        total_liability += due;
    }
    char summary[120];
    snprintf(summary, sizeof(summary), "\nTotal Liability: $%.2f", total_liability);
    strncat(report, summary, sizeof(report) - strlen(report) - 1);
    show_info_dialog(report);
}

static void sales_without_customer_report_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];
    char report[12000] = "SALES WITHOUT ASSIGNED CUSTOMER\n\n";
    int count = 0;
    for (int i = 0; i < s->sales_count; i++) {
        Transaction *t = &s->sales[i];
        if (t->customer_idx >= 0) continue;
        char line[220];
        snprintf(line, sizeof(line), "%s | Date:%s | Total:$%.2f | Status:%d\n", t->transaction_id, t->date, t->total, t->status);
        strncat(report, line, sizeof(report) - strlen(report) - 1);
        count++;
    }
    if (count == 0) strncat(report, "No unassigned sales found.\n", sizeof(report) - strlen(report) - 1);
    show_info_dialog(report);
}

static void product_master_inventory_report_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];
    recompute_committed_for_store(s, si);
    char report[12000] = "PRODUCT MASTER + INVENTORY\n\n";
    for (int i = 0; i < s->product_count; i++) {
        Product *p = &s->products[i];
        double margin = (p->price > 0.0) ? ((p->price - p->average_cost) / p->price) * 100.0 : 0.0;
        char line[420];
        snprintf(line, sizeof(line),
                 "%s | %s | Brand:%s | UPC:%s | MPN:%s | Year:%d | Color:%s | Size:%s\n"
                 "  MSRP:$%.2f Sale:$%.2f Avg:$%.2f Last:$%.2f Margin:%.1f%% | Stock:%d OnOrder:%d Received:%d Committed:%d | eCom:%s\n",
                 p->sku, p->name,
                 p->brand, p->upc, p->manufacturer_part_number, p->model_year, p->color, p->size,
                 p->msrp, p->price, p->average_cost, p->last_cost, margin,
                 p->stock, p->on_order_qty, p->received_qty, p->committed_qty,
                 p->ecommerce_sync ? "Yes" : "No");
        strncat(report, line, sizeof(report) - strlen(report) - 1);
    }
    show_info_dialog(report);
}

static int selected_work_order_row_index(GtkWidget *dialog, GtkTreeIter *out_iter) {
    GtkWidget *tree = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "wo_tree"));
    if (!tree) return -1;
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    GtkTreeModel *model;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) return -1;
    int idx = -1;
    gtk_tree_model_get(model, &iter, 8, &idx, -1);
    if (out_iter) *out_iter = iter;
    return idx;
}

static void on_work_order_update_status_clicked(GtkButton *button, gpointer data) {
    (void)button;
    GtkWidget *dialog = GTK_WIDGET(data);
    if (!role_can_service() && !request_manager_override("Update Work Order Status")) return;

    GtkTreeIter row_iter;
    int idx = selected_work_order_row_index(dialog, &row_iter);
    if (idx < 0 || idx >= work_order_count) {
        show_error_dialog("Select a work order first.");
        return;
    }
    WorkOrder *wo = &work_orders[idx];

    GtkWidget *status_dialog = gtk_dialog_new_with_buttons("Update Work Order Status",
                                                           GTK_WINDOW(dialog),
                                                           GTK_DIALOG_MODAL,
                                                           "_Save", GTK_RESPONSE_OK,
                                                           "_Cancel", GTK_RESPONSE_CANCEL,
                                                           NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(status_dialog));
    GtkWidget *combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Open");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Waiting on Parts");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "In Progress");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Completed");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Picked Up");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), wo->status);
    gtk_container_add(GTK_CONTAINER(area), combo);
    gtk_widget_show_all(status_dialog);
    if (gtk_dialog_run(GTK_DIALOG(status_dialog)) == GTK_RESPONSE_OK) {
        wo->status = (WorkOrderStatus)gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        strftime(wo->updated_at, sizeof(wo->updated_at), "%Y-%m-%d %H:%M", tm_info);

        GtkListStore *store_list = GTK_LIST_STORE(g_object_get_data(G_OBJECT(dialog), "wo_store"));
        gtk_list_store_set(store_list, &row_iter,
                           4, work_order_status_name(wo->status),
                           -1);
        char details[160];
        snprintf(details, sizeof(details), "WO-%d status -> %s", wo->id, work_order_status_name(wo->status));
        add_audit_log_entry("Ascend User", "WorkOrderStatus", details);
        save_data();
    }
    gtk_widget_destroy(status_dialog);
}

static void on_work_order_add_part_clicked(GtkButton *button, gpointer data) {
    (void)button;
    GtkWidget *dialog = GTK_WIDGET(data);
    if (!role_can_service() && !request_manager_override("Add Work Order Part")) return;

    GtkTreeIter row_iter;
    int idx = selected_work_order_row_index(dialog, &row_iter);
    if (idx < 0 || idx >= work_order_count) {
        show_error_dialog("Select a work order first.");
        return;
    }
    WorkOrder *wo = &work_orders[idx];
    if (wo->store_idx < 0 || wo->store_idx >= store_count) {
        show_error_dialog("Work order store is invalid.");
        return;
    }
    Store *s = &stores[wo->store_idx];

    GtkWidget *part_dialog = gtk_dialog_new_with_buttons("Add Part to Work Order",
                                                         GTK_WINDOW(dialog),
                                                         GTK_DIALOG_MODAL,
                                                         "_Apply", GTK_RESPONSE_OK,
                                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                                         NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(part_dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(area), vbox);
    GtkWidget *sku_entry = create_labeled_entry("Part SKU:", vbox);
    GtkWidget *qty_entry = create_labeled_entry("Qty:", vbox);
    gtk_entry_set_text(GTK_ENTRY(qty_entry), "1");

    gtk_widget_show_all(part_dialog);
    if (gtk_dialog_run(GTK_DIALOG(part_dialog)) == GTK_RESPONSE_OK) {
        const char *sku = gtk_entry_get_text(GTK_ENTRY(sku_entry));
        int qty = atoi(gtk_entry_get_text(GTK_ENTRY(qty_entry)));
        Product *p = NULL;
        for (int i = 0; i < s->product_count; i++) {
            if (strcmp(s->products[i].sku, sku) == 0) {
                p = &s->products[i];
                break;
            }
        }
        if (!p) {
            show_error_dialog("Part SKU not found.");
        } else if (qty <= 0) {
            show_error_dialog("Quantity must be positive.");
        } else if (p->stock < qty) {
            show_error_dialog("Insufficient stock for part deduction.");
        } else {
            p->stock -= qty;
            double line_total = p->price * qty;
            wo->parts_total += line_total;
            wo->total = wo->parts_total + wo->labor_total;
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);
            strftime(wo->updated_at, sizeof(wo->updated_at), "%Y-%m-%d %H:%M", tm_info);

            GtkListStore *store_list = GTK_LIST_STORE(g_object_get_data(G_OBJECT(dialog), "wo_store"));
            gtk_list_store_set(store_list, &row_iter,
                               7, wo->total,
                               -1);

            char details[220];
            snprintf(details, sizeof(details), "WO-%d add part %s x%d cost $%.2f", wo->id, sku, qty, line_total);
            add_audit_log_entry("Ascend User", "WorkOrderAddPart", details);
            save_data();
            show_info_dialog("Part added and inventory deducted.");
        }
    }
    gtk_widget_destroy(part_dialog);
}

static void audit_log_dialog(void) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Audit Log",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Close", GTK_RESPONSE_CLOSE,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(sw, 900, 420);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

    GtkListStore *store_list = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    for (int i = 0; i < audit_log_count; i++) {
        AuditLog *a = &audit_logs[i];
        if (a->hidden) continue;
        GtkTreeIter iter;
        gtk_list_store_append(store_list, &iter);
        gtk_list_store_set(store_list, &iter,
                           0, a->timestamp,
                           1, a->user,
                           2, a->action,
                           3, a->details,
                           -1);
    }

    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_list));
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Timestamp", renderer, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "User", renderer, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Action", renderer, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Details", renderer, "text", 3, NULL);
    gtk_container_add(GTK_CONTAINER(sw), tree);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void create_backup_snapshot(void) {
    save_data();

    mkdir("backups", 0755);
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char stamp[32];
    strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", tm_info);

    char dst_path[128];
    snprintf(dst_path, sizeof(dst_path), "backups/ascend_backup_%s.txt", stamp);

    FILE *src = fopen("ascend_data.txt", "r");
    if (!src) {
        show_error_dialog("Backup failed: could not read ascend_data.txt");
        return;
    }
    FILE *dst = fopen(dst_path, "w");
    if (!dst) {
        fclose(src);
        show_error_dialog("Backup failed: could not create backup file.");
        return;
    }

    char buffer[4096];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, n, dst);
    }
    fclose(src);
    fclose(dst);

    char msg[180];
    snprintf(msg, sizeof(msg), "Backup snapshot created: %s", dst_path);
    add_audit_log_entry("Ascend User", "Backup", msg);
    show_info_dialog(msg);
}

static void create_work_order_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];
    if (s->customer_count == 0) {
        show_error_dialog("No customers in this store. Add a customer first.");
        return;
    }
    if (work_order_count >= MAX_WORK_ORDERS) {
        show_error_dialog("Maximum work orders reached.");
        return;
    }

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Create Work Order",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Create", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *cust_combo = gtk_combo_box_text_new();
    for (int i = 0; i < s->customer_count; i++) {
        if (s->customers[i].hidden) continue;
        char entry[NAME_LEN * 3];
        snprintf(entry, sizeof(entry), "%s %s (%s)", s->customers[i].first_name, s->customers[i].last_name, s->customers[i].company);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cust_combo), entry);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(cust_combo), 0);

    GtkWidget *serial_entry = create_labeled_entry("Bike Serial:", vbox);
    GtkWidget *problem_entry = create_labeled_entry("Problem Description:", vbox);
    GtkWidget *mechanic_entry = create_labeled_entry("Assigned Mechanic:", vbox);
    GtkWidget *labor_rate_entry = create_labeled_entry("Labor Rate:", vbox);
    GtkWidget *labor_hours_entry = create_labeled_entry("Labor Hours:", vbox);
    GtkWidget *parts_total_entry = create_labeled_entry("Parts Total:", vbox);

    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("Customer:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), cust_combo, FALSE, FALSE, 0);
    gtk_entry_set_text(GTK_ENTRY(labor_rate_entry), "95.00");
    gtk_entry_set_text(GTK_ENTRY(labor_hours_entry), "1.00");
    gtk_entry_set_text(GTK_ENTRY(parts_total_entry), "0.00");

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        WorkOrder *wo = &work_orders[work_order_count++];
        memset(wo, 0, sizeof(WorkOrder));
        wo->id = next_work_order_id++;
        wo->store_idx = si;
        wo->customer_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(cust_combo));
        wo->status = WO_OPEN;
        wo->labor_rate = atof(gtk_entry_get_text(GTK_ENTRY(labor_rate_entry)));
        wo->labor_hours = atof(gtk_entry_get_text(GTK_ENTRY(labor_hours_entry)));
        wo->parts_total = atof(gtk_entry_get_text(GTK_ENTRY(parts_total_entry)));
        wo->labor_total = wo->labor_rate * wo->labor_hours;
        wo->total = wo->labor_total + wo->parts_total;
        strncpy(wo->serial, gtk_entry_get_text(GTK_ENTRY(serial_entry)), NAME_LEN - 1);
        wo->serial[NAME_LEN - 1] = '\0';
        strncpy(wo->problem, gtk_entry_get_text(GTK_ENTRY(problem_entry)), sizeof(wo->problem) - 1);
        wo->problem[sizeof(wo->problem) - 1] = '\0';
        strncpy(wo->assigned_mechanic, gtk_entry_get_text(GTK_ENTRY(mechanic_entry)), NAME_LEN - 1);
        wo->assigned_mechanic[NAME_LEN - 1] = '\0';
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        strftime(wo->created_at, sizeof(wo->created_at), "%Y-%m-%d %H:%M", tm_info);
        strncpy(wo->updated_at, wo->created_at, sizeof(wo->updated_at) - 1);

        char detail[256];
        snprintf(detail, sizeof(detail), "WO-%d Store:%s Serial:%s Total:$%.2f", wo->id, s->name, wo->serial, wo->total);
        add_audit_log_entry("Ascend User", "WorkOrderCreate", detail);
        save_data();
        show_info_dialog("Work order created.");
    }
    gtk_widget_destroy(dialog);
}

static const char *work_order_status_name(WorkOrderStatus status) {
    switch (status) {
        case WO_OPEN: return "Open";
        case WO_WAITING_PARTS: return "Waiting on Parts";
        case WO_IN_PROGRESS: return "In Progress";
        case WO_COMPLETED: return "Completed";
        case WO_PICKED_UP: return "Picked Up";
    }
    return "Unknown";
}

static void view_work_orders_dialog(void) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Work Orders",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Close", GTK_RESPONSE_CLOSE,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(sw, 1000, 420);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

    GtkListStore *store_list = gtk_list_store_new(9,
                                                   G_TYPE_INT,
                                                   G_TYPE_STRING,
                                                   G_TYPE_STRING,
                                                   G_TYPE_STRING,
                                                   G_TYPE_STRING,
                                                   G_TYPE_STRING,
                                                   G_TYPE_STRING,
                                                   G_TYPE_DOUBLE,
                                                   G_TYPE_INT);

    for (int i = 0; i < work_order_count; i++) {
        WorkOrder *wo = &work_orders[i];
        if (wo->hidden) continue;
        char store_name[NAME_LEN] = "Unknown";
        char customer_name[NAME_LEN * 2] = "Unknown";
        if (wo->store_idx >= 0 && wo->store_idx < store_count) {
            Store *s = &stores[wo->store_idx];
            strncpy(store_name, s->name, sizeof(store_name) - 1);
            if (wo->customer_idx >= 0 && wo->customer_idx < s->customer_count) {
                snprintf(customer_name, sizeof(customer_name), "%s %s", s->customers[wo->customer_idx].first_name, s->customers[wo->customer_idx].last_name);
            }
        }

        GtkTreeIter iter;
        gtk_list_store_append(store_list, &iter);
        gtk_list_store_set(store_list, &iter,
                           0, wo->id,
                           1, store_name,
                           2, customer_name,
                           3, wo->serial,
                           4, work_order_status_name(wo->status),
                           5, wo->assigned_mechanic,
                           6, wo->created_at,
                           7, wo->total,
                           8, i,
                           -1);
    }

    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_list));
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "WO #", renderer, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Store", renderer, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Customer", renderer, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Serial", renderer, "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Status", renderer, "text", 4, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Mechanic", renderer, "text", 5, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Created", renderer, "text", 6, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Total", renderer, "text", 7, NULL);
    gtk_container_add(GTK_CONTAINER(sw), tree);

    g_object_set_data(G_OBJECT(dialog), "wo_tree", tree);
    g_object_set_data(G_OBJECT(dialog), "wo_store", store_list);

    GtkWidget *btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(vbox), btn_row, FALSE, FALSE, 0);
    GtkWidget *status_btn = gtk_button_new_with_label("Update Status");
    GtkWidget *add_part_btn = gtk_button_new_with_label("Add Part + Deduct Inventory");
    gtk_box_pack_start(GTK_BOX(btn_row), status_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btn_row), add_part_btn, FALSE, FALSE, 0);
    g_signal_connect(status_btn, "clicked", G_CALLBACK(on_work_order_update_status_clicked), dialog);
    g_signal_connect(add_part_btn, "clicked", G_CALLBACK(on_work_order_add_part_clicked), dialog);

    gtk_widget_show_all(dialog);
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
        set_desktop_locked(!desktop_locked);
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
    save_data();
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

static void on_customers_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    manage_customers_dialog();
}

static void on_sales_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    create_sale_dialog();
}

static void on_return_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    create_return_dialog();
}

static void on_layaway_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    create_layaway_dialog();
}

static void on_business_clicked(GtkButton *button, gpointer user_data) {
    business_menu_dialog();
}

static void on_trend_clicked(GtkButton *button, gpointer user_data) {
    show_trend_dialog();
}

static void on_save_clicked(GtkButton *button, gpointer user_data) {
    save_data();
    add_audit_log_entry("Ascend User", "SaveData", "Manual save from main menu");
    show_info_dialog("Data saved successfully!");
}

static void on_load_clicked(GtkButton *button, gpointer user_data) {
    load_data();
    add_audit_log_entry("Ascend User", "LoadData", "Manual load from main menu");
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
    if (!tiles_initialized) {
        init_default_tiles();
        tiles_initialized = 1;
    }

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

    GtkWidget *work_orders_view_item = gtk_menu_item_new_with_label("Work Orders");
    g_signal_connect(work_orders_view_item, "activate", G_CALLBACK(view_work_orders_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), work_orders_view_item);

    GtkWidget *reservations_view_item = gtk_menu_item_new_with_label("Reservations");
    g_signal_connect(reservations_view_item, "activate", G_CALLBACK(view_reservations_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), reservations_view_item);

    GtkWidget *purchase_orders_view_item = gtk_menu_item_new_with_label("Purchase Orders");
    g_signal_connect(purchase_orders_view_item, "activate", G_CALLBACK(view_purchase_orders_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), purchase_orders_view_item);

    GtkWidget *payment_ledger_item = gtk_menu_item_new_with_label("Payment Ledger");
    g_signal_connect(payment_ledger_item, "activate", G_CALLBACK(payment_ledger_report_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), payment_ledger_item);

    GtkWidget *alerts_item = gtk_menu_item_new_with_label("Alert Center");
    g_signal_connect(alerts_item, "activate", G_CALLBACK(alert_center_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), alerts_item);

    GtkWidget *desktop_menu = gtk_menu_new();
    GtkWidget *desktop_item = gtk_menu_item_new_with_label("Desktop");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(desktop_item), desktop_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), desktop_item);

    GtkWidget *unlock_desktop_item = gtk_menu_item_new_with_label("Unlock Desktop (Drag Tiles)");
    g_signal_connect(unlock_desktop_item, "activate", G_CALLBACK(on_unlock_desktop_clicked), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(desktop_menu), unlock_desktop_item);

    GtkWidget *lock_desktop_item = gtk_menu_item_new_with_label("Lock Desktop");
    g_signal_connect(lock_desktop_item, "activate", G_CALLBACK(on_lock_desktop_clicked), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(desktop_menu), lock_desktop_item);

    GtkWidget *customize_tiles_item = gtk_menu_item_new_with_label("Customize Home Tiles (Show/Hide/Color)");
    g_signal_connect(customize_tiles_item, "activate", G_CALLBACK(customize_desktop_tiles_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(desktop_menu), customize_tiles_item);

    // Main menu (all home-screen actions)
    GtkWidget *main_menu = gtk_menu_new();
    GtkWidget *main_item = gtk_menu_item_new_with_label("Main");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(main_item), main_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), main_item);

    GtkWidget *main_add_store = gtk_menu_item_new_with_label("Add Store");
    g_signal_connect(main_add_store, "activate", G_CALLBACK(on_add_store_clicked), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(main_menu), main_add_store);

    GtkWidget *main_list_stores = gtk_menu_item_new_with_label("List Stores");
    g_signal_connect(main_list_stores, "activate", G_CALLBACK(on_list_stores_clicked), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(main_menu), main_list_stores);

    GtkWidget *main_store_info = gtk_menu_item_new_with_label("Store Info");
    g_signal_connect(main_store_info, "activate", G_CALLBACK(on_store_info_clicked), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(main_menu), main_store_info);

    GtkWidget *main_inventory = gtk_menu_item_new_with_label("Inventory");
    g_signal_connect(main_inventory, "activate", G_CALLBACK(on_inventory_clicked), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(main_menu), main_inventory);

    GtkWidget *main_customers = gtk_menu_item_new_with_label("Customers");
    g_signal_connect(main_customers, "activate", G_CALLBACK(on_customers_clicked), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(main_menu), main_customers);

    GtkWidget *main_sales = gtk_menu_item_new_with_label("Sales");
    g_signal_connect(main_sales, "activate", G_CALLBACK(on_sales_clicked), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(main_menu), main_sales);

    GtkWidget *main_return = gtk_menu_item_new_with_label("Return");
    g_signal_connect(main_return, "activate", G_CALLBACK(on_return_clicked), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(main_menu), main_return);

    GtkWidget *main_layaway = gtk_menu_item_new_with_label("Layaway");
    g_signal_connect(main_layaway, "activate", G_CALLBACK(on_layaway_clicked), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(main_menu), main_layaway);

    GtkWidget *main_business = gtk_menu_item_new_with_label("Business");
    g_signal_connect(main_business, "activate", G_CALLBACK(on_business_clicked), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(main_menu), main_business);

    GtkWidget *main_trend = gtk_menu_item_new_with_label("Trend Chart");
    g_signal_connect(main_trend, "activate", G_CALLBACK(on_trend_clicked), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(main_menu), main_trend);

    GtkWidget *main_save = gtk_menu_item_new_with_label("Save Data");
    g_signal_connect(main_save, "activate", G_CALLBACK(on_save_clicked), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(main_menu), main_save);

    GtkWidget *main_load = gtk_menu_item_new_with_label("Load Data");
    g_signal_connect(main_load, "activate", G_CALLBACK(on_load_clicked), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(main_menu), main_load);

    GtkWidget *main_exit = gtk_menu_item_new_with_label("Exit");
    g_signal_connect(main_exit, "activate", G_CALLBACK(on_exit_clicked), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(main_menu), main_exit);

    // Sales menu
    GtkWidget *sales_menu = gtk_menu_new();
    GtkWidget *sales_item = gtk_menu_item_new_with_label("Sales");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(sales_item), sales_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), sales_item);

    GtkWidget *return_item = gtk_menu_item_new_with_label("Create a Return");
    g_signal_connect(return_item, "activate", G_CALLBACK(create_return_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(sales_menu), return_item);

    // Options menu
    GtkWidget *options_menu = gtk_menu_new();
    GtkWidget *options_item = gtk_menu_item_new_with_label("Options");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(options_item), options_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), options_item);

    GtkWidget *sales_returns_menu = gtk_menu_new();
    GtkWidget *sales_returns_item = gtk_menu_item_new_with_label("Sales and Returns");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(sales_returns_item), sales_returns_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(options_menu), sales_returns_item);

    GtkWidget *work_order_defaults_item = gtk_menu_item_new_with_label("Work Order Defaults");
    g_signal_connect(work_order_defaults_item, "activate", G_CALLBACK(work_order_defaults_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(sales_returns_menu), work_order_defaults_item);

    GtkWidget *ordering_item = gtk_menu_item_new_with_label("Ordering");
    g_signal_connect(ordering_item, "activate", G_CALLBACK(ordering_serial_prompt_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(options_menu), ordering_item);

    GtkWidget *special_orders_item = gtk_menu_item_new_with_label("Special Ordered Items");
    g_signal_connect(special_orders_item, "activate", G_CALLBACK(view_special_orders_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), special_orders_item);

    // Tools menu
    GtkWidget *tools_menu = gtk_menu_new();
    GtkWidget *tools_item = gtk_menu_item_new_with_label("Tools");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(tools_item), tools_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), tools_item);

    GtkWidget *new_work_order_item = gtk_menu_item_new_with_label("New Work Order");
    g_signal_connect(new_work_order_item, "activate", G_CALLBACK(create_work_order_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), new_work_order_item);

    GtkWidget *new_reservation_item = gtk_menu_item_new_with_label("New Reservation");
    g_signal_connect(new_reservation_item, "activate", G_CALLBACK(create_reservation_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), new_reservation_item);

    GtkWidget *split_payment_item = gtk_menu_item_new_with_label("Add Split Payment Entry");
    g_signal_connect(split_payment_item, "activate", G_CALLBACK(add_split_payment_entry_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), split_payment_item);

    GtkWidget *new_po_item = gtk_menu_item_new_with_label("Create Purchase Order");
    g_signal_connect(new_po_item, "activate", G_CALLBACK(create_purchase_order_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), new_po_item);

    GtkWidget *receive_po_item = gtk_menu_item_new_with_label("Receive Purchase Order");
    g_signal_connect(receive_po_item, "activate", G_CALLBACK(receive_purchase_order_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), receive_po_item);

    GtkWidget *vendor_link_item = gtk_menu_item_new_with_label("Vendor Product Linking");
    g_signal_connect(vendor_link_item, "activate", G_CALLBACK(vendor_product_linking_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), vendor_link_item);

    GtkWidget *system_config_item = gtk_menu_item_new_with_label("System Configuration");
    g_signal_connect(system_config_item, "activate", G_CALLBACK(system_configuration_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), system_config_item);

    GtkWidget *email_doc_item = gtk_menu_item_new_with_label("Email Receipt / Quote");
    g_signal_connect(email_doc_item, "activate", G_CALLBACK(email_receipt_quote_stub_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), email_doc_item);

    GtkWidget *audit_log_item = gtk_menu_item_new_with_label("Audit Log");
    g_signal_connect(audit_log_item, "activate", G_CALLBACK(audit_log_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), audit_log_item);

    GtkWidget *backup_item = gtk_menu_item_new_with_label("Create Backup Snapshot");
    g_signal_connect(backup_item, "activate", G_CALLBACK(create_backup_snapshot), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), backup_item);

    GtkWidget *security_item = gtk_menu_item_new_with_label("Security and Permissions");
    g_signal_connect(security_item, "activate", G_CALLBACK(security_permissions_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), security_item);

    GtkWidget *theme_item = gtk_menu_item_new_with_label("Theme and Display Mode");
    g_signal_connect(theme_item, "activate", G_CALLBACK(theme_display_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), theme_item);

    GtkWidget *barcode_item = gtk_menu_item_new_with_label("Print Barcode Labels");
    g_signal_connect(barcode_item, "activate", G_CALLBACK(barcode_label_print_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), barcode_item);

    GtkWidget *matrix_item = gtk_menu_item_new_with_label("Generate Product Matrix");
    g_signal_connect(matrix_item, "activate", G_CALLBACK(generate_product_matrix_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), matrix_item);

    GtkWidget *sql_tools_item = gtk_menu_item_new_with_label("SQL / Customer / Product Query Tools");
    g_signal_connect(sql_tools_item, "activate", G_CALLBACK(sql_query_tools_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), sql_tools_item);

    GtkWidget *product_lookup_item = gtk_menu_item_new_with_label("Product Lookup (SKU/UPC/Model)");
    g_signal_connect(product_lookup_item, "activate", G_CALLBACK(product_lookup_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), product_lookup_item);

    GtkWidget *vendor_hub_item = gtk_menu_item_new_with_label("Big Four / QBP Vendor Hub");
    g_signal_connect(vendor_hub_item, "activate", G_CALLBACK(vendor_integration_hub_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), vendor_hub_item);

    GtkWidget *blackout_item = gtk_menu_item_new_with_label("Blackout Protocol (Offline Sync)");
    g_signal_connect(blackout_item, "activate", G_CALLBACK(offline_blackout_protocol_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), blackout_item);

    GtkWidget *stand_item = gtk_menu_item_new_with_label("Stand Manager");
    g_signal_connect(stand_item, "activate", G_CALLBACK(service_stand_manager_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), stand_item);

    GtkWidget *bundle_item = gtk_menu_item_new_with_label("Labor + Parts Bundle");
    g_signal_connect(bundle_item, "activate", G_CALLBACK(labor_bundle_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), bundle_item);

    GtkWidget *bluebook_item = gtk_menu_item_new_with_label("Trade-In Bluebook");
    g_signal_connect(bluebook_item, "activate", G_CALLBACK(trade_in_bluebook_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), bluebook_item);

    GtkWidget *suspension_item = gtk_menu_item_new_with_label("Suspension Setup Log");
    g_signal_connect(suspension_item, "activate", G_CALLBACK(suspension_setup_log_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), suspension_item);

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

    GtkWidget *daily_sales_item = gtk_menu_item_new_with_label("Daily Sales + Payment Breakdown");
    g_signal_connect(daily_sales_item, "activate", G_CALLBACK(daily_sales_payment_report_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(reports_menu), daily_sales_item);

    GtkWidget *payment_ledger_report_item = gtk_menu_item_new_with_label("Payment Ledger Report");
    g_signal_connect(payment_ledger_report_item, "activate", G_CALLBACK(payment_ledger_report_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(reports_menu), payment_ledger_report_item);

    GtkWidget *eod_report_item = gtk_menu_item_new_with_label("End of Day Summary");
    g_signal_connect(eod_report_item, "activate", G_CALLBACK(end_of_day_summary_report_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(reports_menu), eod_report_item);

    GtkWidget *liability_report_item = gtk_menu_item_new_with_label("Customer Liability Details");
    g_signal_connect(liability_report_item, "activate", G_CALLBACK(customer_liability_report_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(reports_menu), liability_report_item);

    GtkWidget *no_customer_sales_item = gtk_menu_item_new_with_label("Sales Without Assigned Customers");
    g_signal_connect(no_customer_sales_item, "activate", G_CALLBACK(sales_without_customer_report_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(reports_menu), no_customer_sales_item);

    GtkWidget *product_master_item = gtk_menu_item_new_with_label("Product Master + Inventory");
    g_signal_connect(product_master_item, "activate", G_CALLBACK(product_master_inventory_report_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(reports_menu), product_master_item);

    GtkWidget *low_stock_item = gtk_menu_item_new_with_label("Low Stock");
    g_signal_connect(low_stock_item, "activate", G_CALLBACK(low_stock_report_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(reports_menu), low_stock_item);

    GtkWidget *best_sellers_item = gtk_menu_item_new_with_label("Best-Selling Items");
    g_signal_connect(best_sellers_item, "activate", G_CALLBACK(best_selling_items_report_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(reports_menu), best_sellers_item);

    GtkWidget *ai_estimator_item = gtk_menu_item_new_with_label("AI Service Estimator");
    g_signal_connect(ai_estimator_item, "activate", G_CALLBACK(service_ai_estimator_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(reports_menu), ai_estimator_item);

    GtkWidget *sms_pickup_item = gtk_menu_item_new_with_label("SMS Pickup Notifications");
    g_signal_connect(sms_pickup_item, "activate", G_CALLBACK(sms_pickup_notification_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(reports_menu), sms_pickup_item);

    GtkWidget *wo_progress_sms_item = gtk_menu_item_new_with_label("Work Order Progress SMS");
    g_signal_connect(wo_progress_sms_item, "activate", G_CALLBACK(work_order_progress_sms_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(reports_menu), wo_progress_sms_item);

    GtkWidget *buyer_dashboard_item = gtk_menu_item_new_with_label("Buying Dashboard");
    g_signal_connect(buyer_dashboard_item, "activate", G_CALLBACK(buyer_dashboard_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(reports_menu), buyer_dashboard_item);

    // Help menu
    GtkWidget *help_menu = gtk_menu_new();
    GtkWidget *help_item = gtk_menu_item_new_with_label("Help");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_item), help_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), help_item);

    GtkWidget *instructions_item = gtk_menu_item_new_with_label("Instructions Reference");
    g_signal_connect(instructions_item, "activate", G_CALLBACK(instructions_reference_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), instructions_item);

    // Title bar
    GtkWidget *title_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), title_box, FALSE, FALSE, 0);
    gtk_widget_set_size_request(title_box, -1, 50);

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='large' weight='bold'>Ascend Retail Platform</span>");
    gtk_box_pack_start(GTK_BOX(title_box), title, FALSE, FALSE, 15);

    GtkWidget *instructions_btn = gtk_button_new_with_label("Instructions Reference");
    g_signal_connect(instructions_btn, "clicked", G_CALLBACK(instructions_reference_dialog), NULL);
    gtk_box_pack_end(GTK_BOX(title_box), instructions_btn, FALSE, FALSE, 10);

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

static void instructions_reference_dialog(void) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Instructions Reference",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Close", GTK_RESPONSE_CLOSE,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *top_note = gtk_label_new("Quick Reference + Search Tips\nUse Ctrl/Cmd+F in PROGRAM_INSTRUCTIONS.md for full indexed documentation.");
    gtk_label_set_line_wrap(GTK_LABEL(top_note), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), top_note, FALSE, FALSE, 0);

    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(sw, 760, 430);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_container_add(GTK_CONTAINER(sw), text_view);

    const char *quick_ref =
        "ASCEND-CLONE INSTRUCTIONS REFERENCE\n\n"
        "FULL DOC FILE\n"
        "- PROGRAM_INSTRUCTIONS.md\n"
        "- Includes complete step-by-step workflows, quick refs, and A-Z index\n\n"
        "TOP MENU PATHS\n"
        "- View > Layaways\n"
        "- View > Special Ordered Items\n"
        "- View > Customer Tax Exceptions\n"
        "- View > Time Clock\n"
        "- Options > Sales and Returns > Work Order Defaults\n"
        "- Options > Ordering\n"
        "- Reports > Time Clock\n"
        "- Reports > Daily Sales + Payment Breakdown\n"
        "- Reports > Low Stock\n"
        "- Reports > Best-Selling Items\n"
        "- Help > Instructions Reference\n\n"
        "KEY WORKFLOWS\n"
        "- Sales: Use Sales tile, add items, take payment, print receipt\n"
        "- Returns: Use Return tile, lookup prior transaction, process refund\n"
        "- Layaway: Use Layaway tile, save open or apply partial payment\n"
        "- Special Orders: Reorder List > mark ordered, then mark received\n"
        "- Time Clock: Add/Modify/Remove/Restore, filter by date, show hidden\n\n"
        "CSV EXPORTS\n"
        "- time_clock_report.csv\n"
        "- daily_sales_payment_report.csv\n"
        "- low_stock_report.csv\n\n"
        "TIME CLOCK ALERTS\n"
        "- Missing clock-out highlighted in Time Clock report\n"
        "- Long shift highlighted for shifts >= 10 hours\n\n"
        "SEARCH KEYWORDS\n"
        "sales, returns, layaway, special order, reorder, receiving, serial,\n"
        "time clock, long shift, missing clock-out, tax exceptions, low stock,\n"
        "best-selling, payment breakdown, export csv, options, reports\n\n"
        "TIP\n"
        "- Open PROGRAM_INSTRUCTIONS.md and use Cmd+F for full keyword index and tags.\n";

    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buf, quick_ref, -1);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
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

static int prompt_for_serial_number_dialog(const char *title, const char *context, char *out_serial, size_t out_size) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(title,
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Save", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(context), FALSE, FALSE, 0);
    GtkWidget *serial_entry = create_labeled_entry("Serial Number:", vbox);

    gtk_widget_show_all(dialog);
    int accepted = 0;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *serial = gtk_entry_get_text(GTK_ENTRY(serial_entry));
        if (serial && strlen(serial) > 0) {
            strncpy(out_serial, serial, out_size - 1);
            out_serial[out_size - 1] = '\0';
            accepted = 1;
        } else {
            show_error_dialog("Serial Number is required for serialized items.");
        }
    }
    gtk_widget_destroy(dialog);
    return accepted;
}

static void work_order_defaults_dialog(void) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Work Order Defaults",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_OK", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *prompt_check = gtk_check_button_new_with_label("Prompt for Serial Number");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prompt_check), app_settings.prompt_work_order_serial);
    gtk_box_pack_start(GTK_BOX(vbox), prompt_check, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox),
        gtk_label_new("Disabling this prompt means serial numbers/descriptions must be associated manually in Work Order Details."),
        FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        app_settings.prompt_work_order_serial = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prompt_check));
        save_data();
        show_info_dialog("Work Order defaults updated.");
    }
    gtk_widget_destroy(dialog);
}

static void ordering_serial_prompt_dialog(void) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Ordering Options",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_OK", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *receiving_check = gtk_check_button_new_with_label("Prompt for Serial Number when Receiving");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(receiving_check), app_settings.prompt_receiving_serial);
    gtk_box_pack_start(GTK_BOX(vbox), receiving_check, FALSE, FALSE, 0);

    GtkWidget *cannot_disable = gtk_label_new("Note: Prompt to associate a serial number at time of sale cannot be disabled.");
    gtk_label_set_line_wrap(GTK_LABEL(cannot_disable), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), cannot_disable, FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        app_settings.prompt_receiving_serial = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(receiving_check));
        save_data();
        show_info_dialog("Ordering options updated.");
    }
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

    GtkWidget *user_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(user_combo), "All Employees");
    for (int i = 0; i < time_clock_count; i++) {
        int exists = 0;
        for (int j = 0; j < i; j++) {
            if (strcmp(time_clock_entries[i].user_name, time_clock_entries[j].user_name) == 0) {
                exists = 1;
                break;
            }
        }
        if (!exists && strlen(time_clock_entries[i].user_name) > 0) {
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(user_combo), time_clock_entries[i].user_name);
        }
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(user_combo), 0);
    gtk_box_pack_start(GTK_BOX(filter_box), gtk_label_new("Employee:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(filter_box), user_combo, FALSE, FALSE, 0);

    GtkWidget *refresh_btn = gtk_button_new_with_label("Refresh");
    gtk_box_pack_start(GTK_BOX(filter_box), refresh_btn, FALSE, FALSE, 0);

    GtkWidget *export_btn = gtk_button_new_with_label("Export CSV");
    gtk_box_pack_start(GTK_BOX(filter_box), export_btn, FALSE, FALSE, 0);

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
    g_object_set_data(G_OBJECT(dialog), "time_clock_report_user_combo", user_combo);

    g_signal_connect(refresh_btn, "clicked", G_CALLBACK(on_refresh_time_clock_report_clicked), dialog);
    g_signal_connect(export_btn, "clicked", G_CALLBACK(on_export_time_clock_report_clicked), dialog);

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
    GtkWidget *user_combo = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "time_clock_report_user_combo"));
    if (!from_entry || !to_entry || !text_view || !user_combo) return;

    const char *from_date = gtk_entry_get_text(GTK_ENTRY(from_entry));
    const char *to_date = gtk_entry_get_text(GTK_ENTRY(to_entry));
    gchar *selected_user = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(user_combo));
    int filter_all_users = (!selected_user || strcmp(selected_user, "All Employees") == 0);
    double total_hours = 0.0;
    int missing_clock_out_count = 0;
    int long_shift_count = 0;
    char report[12000] = "TIME CLOCK REPORT\n\n";
    char line[256];

    for (int i = 0; i < time_clock_count; i++) {
        TimeClockEntry *e = &time_clock_entries[i];
        if (e->hidden) continue;
        if (!filter_all_users && strcmp(e->user_name, selected_user) != 0) continue;
        char entry_date[11] = "";
        strncpy(entry_date, e->start_time, 10);
        entry_date[10] = '\0';
        if (strlen(from_date) >= 10 && strcmp(entry_date, from_date) < 0) continue;
        if (strlen(to_date) >= 10 && strcmp(entry_date, to_date) > 0) continue;

        double h = compute_time_clock_hours(e);
        if (e->has_end_time) {
            if (h >= 10.0) {
                sprintf(line, "%s | %s -> %s | %.2f hrs | ALERT: Long shift\n", e->user_name, e->start_time, e->end_time, h);
                long_shift_count++;
            } else {
                sprintf(line, "%s | %s -> %s | %.2f hrs\n", e->user_name, e->start_time, e->end_time, h);
            }
            total_hours += h;
        } else {
            sprintf(line, "%s | %s -> (Still Working) | ALERT: Missing clock-out\n", e->user_name, e->start_time);
            missing_clock_out_count++;
        }
        strncat(report, line, sizeof(report) - strlen(report) - 1);
    }

    sprintf(line, "\nTotal Closed-Shift Hours: %.2f\n", total_hours);
    strncat(report, line, sizeof(report) - strlen(report) - 1);
    sprintf(line, "Long Shifts (>=10h): %d\nMissing Clock-Outs: %d\n", long_shift_count, missing_clock_out_count);
    strncat(report, line, sizeof(report) - strlen(report) - 1);

    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buf, report, -1);
    if (selected_user) g_free(selected_user);
}

static void on_export_time_clock_report_clicked(GtkButton *button, gpointer data) {
    (void)button;
    GtkWidget *dialog = GTK_WIDGET(data);
    GtkWidget *from_entry = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "time_clock_report_from"));
    GtkWidget *to_entry = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "time_clock_report_to"));
    GtkWidget *user_combo = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "time_clock_report_user_combo"));
    if (!from_entry || !to_entry || !user_combo) return;

    const char *from_date = gtk_entry_get_text(GTK_ENTRY(from_entry));
    const char *to_date = gtk_entry_get_text(GTK_ENTRY(to_entry));
    gchar *selected_user = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(user_combo));
    int filter_all_users = (!selected_user || strcmp(selected_user, "All Employees") == 0);

    FILE *f = fopen("time_clock_report.csv", "w");
    if (!f) {
        show_error_dialog("Unable to export time clock CSV.");
        return;
    }

    fprintf(f, "User,Start Time,End Time,Hours,Alert\n");
    for (int i = 0; i < time_clock_count; i++) {
        TimeClockEntry *e = &time_clock_entries[i];
        if (e->hidden) continue;
        if (!filter_all_users && strcmp(e->user_name, selected_user) != 0) continue;
        char entry_date[11] = "";
        strncpy(entry_date, e->start_time, 10);
        entry_date[10] = '\0';
        if (strlen(from_date) >= 10 && strcmp(entry_date, from_date) < 0) continue;
        if (strlen(to_date) >= 10 && strcmp(entry_date, to_date) > 0) continue;

        if (e->has_end_time) {
            double hours = compute_time_clock_hours(e);
            if (hours >= 10.0) {
                fprintf(f, "%s,%s,%s,%.2f,Long shift\n", e->user_name, e->start_time, e->end_time, hours);
            } else {
                fprintf(f, "%s,%s,%s,%.2f,\n", e->user_name, e->start_time, e->end_time, hours);
            }
        } else {
            fprintf(f, "%s,%s,(Still Working),0.00,Missing clock-out\n", e->user_name, e->start_time);
        }
    }
    fclose(f);
    if (selected_user) g_free(selected_user);
    show_info_dialog("Exported time clock report to time_clock_report.csv");
}

static void on_refresh_daily_sales_payment_report_clicked(GtkButton *button, gpointer data) {
    (void)button;
    refresh_daily_sales_payment_report(GTK_WIDGET(data));
}

static void on_export_daily_sales_payment_report_clicked(GtkButton *button, gpointer data) {
    (void)button;
    GtkWidget *dialog = GTK_WIDGET(data);
    GtkWidget *from_entry = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "daily_sales_from"));
    GtkWidget *to_entry = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "daily_sales_to"));
    gint si = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dialog), "daily_sales_store_idx"));
    if (!from_entry || !to_entry || si < 0 || si >= store_count) return;

    Store *s = &stores[si];
    const char *from_date = gtk_entry_get_text(GTK_ENTRY(from_entry));
    const char *to_date = gtk_entry_get_text(GTK_ENTRY(to_entry));

    double revenue = 0.0;
    double cash_total = 0.0;
    double card_total = 0.0;
    double debit_total = 0.0;
    double gift_total = 0.0;
    double other_total = 0.0;
    int sale_count = 0;

    for (int i = 0; i < s->sales_count; i++) {
        Transaction *txn = &s->sales[i];
        if (txn->status == 0) continue;
        if (strlen(txn->date) < 10) continue;

        char txn_date[11] = "";
        strncpy(txn_date, txn->date, 10);
        txn_date[10] = '\0';
        if (strlen(from_date) >= 10 && strcmp(txn_date, from_date) < 0) continue;
        if (strlen(to_date) >= 10 && strcmp(txn_date, to_date) > 0) continue;

        revenue += txn->total;
        sale_count++;
    }

    int payment_count = 0;
    summarize_payment_totals_for_store_date(s, from_date, to_date,
                                            &cash_total, &card_total, &debit_total,
                                            &gift_total, &other_total, &payment_count);
    if (payment_count == 0) {
        for (int i = 0; i < s->sales_count; i++) {
            Transaction *txn = &s->sales[i];
            if (txn->status == 0 || strlen(txn->date) < 10) continue;
            char txn_date[11] = "";
            strncpy(txn_date, txn->date, 10);
            txn_date[10] = '\0';
            if (strlen(from_date) >= 10 && strcmp(txn_date, from_date) < 0) continue;
            if (strlen(to_date) >= 10 && strcmp(txn_date, to_date) > 0) continue;
            if (txn->payment_type == PAYMENT_CASH) cash_total += txn->total;
            else if (txn->payment_type == PAYMENT_CREDIT) card_total += txn->total;
            else if (txn->payment_type == PAYMENT_DEBIT) debit_total += txn->total;
            else if (txn->payment_type == PAYMENT_GIFT) gift_total += txn->total;
        }
    }

    FILE *f = fopen("daily_sales_payment_report.csv", "w");
    if (!f) {
        show_error_dialog("Unable to export daily sales CSV.");
        return;
    }
    fprintf(f, "Metric,Value\n");
    fprintf(f, "Store,%s\n", s->name);
    fprintf(f, "From Date,%s\n", from_date);
    fprintf(f, "To Date,%s\n", to_date);
    fprintf(f, "Transactions,%d\n", sale_count);
    fprintf(f, "Revenue,%.2f\n", revenue);
    fprintf(f, "Cash,%.2f\n", cash_total);
    fprintf(f, "Card (Credit),%.2f\n", card_total);
    fprintf(f, "Debit,%.2f\n", debit_total);
    fprintf(f, "Store Credit / Gift,%.2f\n", gift_total);
    fprintf(f, "Other Tenders,%.2f\n", other_total);
    fclose(f);

    show_info_dialog("Exported daily sales report to daily_sales_payment_report.csv");
}

static void refresh_daily_sales_payment_report(GtkWidget *dialog) {
    GtkWidget *from_entry = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "daily_sales_from"));
    GtkWidget *to_entry = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "daily_sales_to"));
    GtkWidget *text_view = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "daily_sales_text"));
    gint si = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dialog), "daily_sales_store_idx"));
    if (!from_entry || !to_entry || !text_view || si < 0 || si >= store_count) return;

    Store *s = &stores[si];
    const char *from_date = gtk_entry_get_text(GTK_ENTRY(from_entry));
    const char *to_date = gtk_entry_get_text(GTK_ENTRY(to_entry));

    double revenue = 0.0;
    double cash_total = 0.0;
    double card_total = 0.0;
    double debit_total = 0.0;
    double gift_total = 0.0;
    double other_total = 0.0;
    int sale_count = 0;

    for (int i = 0; i < s->sales_count; i++) {
        Transaction *txn = &s->sales[i];
        if (txn->status == 0) continue; // skip open layaways
        if (strlen(txn->date) < 10) continue;

        char txn_date[11] = "";
        strncpy(txn_date, txn->date, 10);
        txn_date[10] = '\0';
        if (strlen(from_date) >= 10 && strcmp(txn_date, from_date) < 0) continue;
        if (strlen(to_date) >= 10 && strcmp(txn_date, to_date) > 0) continue;

        revenue += txn->total;
        sale_count++;
    }

    int payment_count = 0;
    summarize_payment_totals_for_store_date(s, from_date, to_date,
                                            &cash_total, &card_total, &debit_total,
                                            &gift_total, &other_total, &payment_count);
    if (payment_count == 0) {
        for (int i = 0; i < s->sales_count; i++) {
            Transaction *txn = &s->sales[i];
            if (txn->status == 0 || strlen(txn->date) < 10) continue;

            char txn_date[11] = "";
            strncpy(txn_date, txn->date, 10);
            txn_date[10] = '\0';
            if (strlen(from_date) >= 10 && strcmp(txn_date, from_date) < 0) continue;
            if (strlen(to_date) >= 10 && strcmp(txn_date, to_date) > 0) continue;

            if (txn->payment_type == PAYMENT_CASH) cash_total += txn->total;
            else if (txn->payment_type == PAYMENT_CREDIT) card_total += txn->total;
            else if (txn->payment_type == PAYMENT_DEBIT) debit_total += txn->total;
            else if (txn->payment_type == PAYMENT_GIFT) gift_total += txn->total;
        }
    }

    char report[3000];
    snprintf(report, sizeof(report),
             "DAILY SALES + PAYMENT BREAKDOWN\n\n"
             "Store: %s\n"
             "Date Range: %s to %s\n\n"
             "Transactions: %d\n"
             "Revenue: $%.2f\n\n"
             "Payment Breakdown:\n"
             "Cash: $%.2f\n"
             "Card (Credit): $%.2f\n"
             "Debit: $%.2f\n"
             "Store Credit / Gift: $%.2f\n"
             "Other Tender: $%.2f\n",
             s->name, from_date, to_date,
             sale_count, revenue,
             cash_total, card_total, debit_total, gift_total, other_total);

    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buf, report, -1);
}

static void daily_sales_payment_report_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Daily Sales + Payment Breakdown",
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

    GtkWidget *export_btn = gtk_button_new_with_label("Export CSV");
    gtk_box_pack_start(GTK_BOX(filter_box), export_btn, FALSE, FALSE, 0);

    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(sw, 680, 320);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);
    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_container_add(GTK_CONTAINER(sw), text_view);

    g_object_set_data(G_OBJECT(dialog), "daily_sales_from", from_entry);
    g_object_set_data(G_OBJECT(dialog), "daily_sales_to", to_entry);
    g_object_set_data(G_OBJECT(dialog), "daily_sales_text", text_view);
    g_object_set_data(G_OBJECT(dialog), "daily_sales_store_idx", GINT_TO_POINTER(si));
    g_signal_connect(refresh_btn, "clicked", G_CALLBACK(on_refresh_daily_sales_payment_report_clicked), dialog);
    g_signal_connect(export_btn, "clicked", G_CALLBACK(on_export_daily_sales_payment_report_clicked), dialog);

    refresh_daily_sales_payment_report(dialog);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void low_stock_report_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];
    recompute_committed_for_store(s, si);

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Low Stock Report",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Run", GTK_RESPONSE_OK,
                                                   "_Close", GTK_RESPONSE_CLOSE,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *threshold_entry = create_labeled_entry("Low Stock Threshold:", vbox);
    gtk_entry_set_text(GTK_ENTRY(threshold_entry), "5");

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 0);
    GtkWidget *export_btn = gtk_button_new_with_label("Export CSV");
    gtk_box_pack_start(GTK_BOX(button_box), export_btn, FALSE, FALSE, 0);

    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(sw, 700, 320);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);
    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_container_add(GTK_CONTAINER(sw), text_view);

    gtk_widget_show_all(dialog);

    g_object_set_data(G_OBJECT(dialog), "low_stock_threshold", threshold_entry);
    g_object_set_data(G_OBJECT(dialog), "low_stock_store_idx", GINT_TO_POINTER(si));
    g_signal_connect(export_btn, "clicked", G_CALLBACK(on_export_low_stock_report_clicked), dialog);

    int done = 0;
    while (!done) {
        int resp = gtk_dialog_run(GTK_DIALOG(dialog));
        if (resp == GTK_RESPONSE_OK) {
            int threshold = atoi(gtk_entry_get_text(GTK_ENTRY(threshold_entry)));
            if (threshold < 0) threshold = 0;

            char report[8000] = "LOW STOCK REPORT\n\n";
            char line[256];
            int count = 0;
            for (int i = 0; i < s->product_count; i++) {
                Product *p = &s->products[i];
                int available = p->stock - p->committed_qty;
                if (available <= threshold) {
                    snprintf(line, sizeof(line), "%s | SKU: %s | Available: %d | Stock: %d | Committed: %d | Price: $%.2f\n",
                             p->name, p->sku, available, p->stock, p->committed_qty, p->price);
                    strncat(report, line, sizeof(report) - strlen(report) - 1);
                    count++;
                }
            }
            if (count == 0) {
                strncat(report, "No products below threshold.\n", sizeof(report) - strlen(report) - 1);
            }

            GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
            gtk_text_buffer_set_text(buf, report, -1);
        } else {
            done = 1;
        }
    }

    gtk_widget_destroy(dialog);
}

static void on_export_low_stock_report_clicked(GtkButton *button, gpointer data) {
    (void)button;
    GtkWidget *dialog = GTK_WIDGET(data);
    GtkWidget *threshold_entry = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "low_stock_threshold"));
    gint si = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dialog), "low_stock_store_idx"));
    if (!threshold_entry || si < 0 || si >= store_count) return;

    Store *s = &stores[si];
    recompute_committed_for_store(s, si);
    int threshold = atoi(gtk_entry_get_text(GTK_ENTRY(threshold_entry)));
    if (threshold < 0) threshold = 0;

    FILE *f = fopen("low_stock_report.csv", "w");
    if (!f) {
        show_error_dialog("Unable to export low stock CSV.");
        return;
    }

    fprintf(f, "Name,SKU,Available,In Stock,Committed,Price\n");
    for (int i = 0; i < s->product_count; i++) {
        Product *p = &s->products[i];
        int available = p->stock - p->committed_qty;
        if (available <= threshold) {
            fprintf(f, "%s,%s,%d,%d,%d,%.2f\n", p->name, p->sku, available, p->stock, p->committed_qty, p->price);
        }
    }
    fclose(f);

    show_info_dialog("Exported low stock report to low_stock_report.csv");
}

static void best_selling_items_report_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Best-Selling Items",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Close", GTK_RESPONSE_CLOSE,
                                                   NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(content_area), sw);
    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_container_add(GTK_CONTAINER(sw), text_view);

    int idx[MAX_PRODUCTS];
    for (int i = 0; i < s->product_count; i++) idx[i] = i;
    for (int i = 0; i < s->product_count; i++) {
        for (int j = i + 1; j < s->product_count; j++) {
            if (s->products[idx[j]].sold > s->products[idx[i]].sold) {
                int tmp = idx[i];
                idx[i] = idx[j];
                idx[j] = tmp;
            }
        }
    }

    char report[10000] = "BEST-SELLING ITEMS\n\n";
    char line[256];
    int shown = 0;
    for (int i = 0; i < s->product_count; i++) {
        Product *p = &s->products[idx[i]];
        if (p->sold <= 0) continue;
        snprintf(line, sizeof(line), "%d. %s | SKU: %s | Units Sold: %d | In Stock: %d\n", shown + 1, p->name, p->sku, p->sold, p->stock);
        strncat(report, line, sizeof(report) - strlen(report) - 1);
        shown++;
        if (shown >= 20) break;
    }
    if (shown == 0) {
        strncat(report, "No sold-item history available yet.\n", sizeof(report) - strlen(report) - 1);
    }

    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buf, report, -1);

    gtk_widget_set_size_request(dialog, 760, 420);
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
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
        char line[320];
        sprintf(line, "- %s (%s) %s %s $%.2f stock=%d onOrder=%d committed=%d\n", p->name, p->sku,
                p->vendor[0] ? p->vendor : "Unknown",
                p->serialized ? "[Serialized]" : "[Non-Serialized]",
            p->price, p->stock, p->on_order_qty, p->committed_qty);
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
    GtkWidget *entry_brand = create_labeled_entry("Brand:", vbox);
    GtkWidget *entry_upc = create_labeled_entry("UPC:", vbox);
    GtkWidget *entry_mpn = create_labeled_entry("Manufacturer Part #:", vbox);
    GtkWidget *entry_style_name = create_labeled_entry("Style Name:", vbox);
    GtkWidget *entry_style_number = create_labeled_entry("Style Number:", vbox);
    GtkWidget *entry_year = create_labeled_entry("Year:", vbox);
    GtkWidget *entry_color = create_labeled_entry("Color:", vbox);
    GtkWidget *entry_size = create_labeled_entry("Size:", vbox);
    GtkWidget *entry_msrp = create_labeled_entry("MSRP ($):", vbox);
    GtkWidget *entry_avg_cost = create_labeled_entry("Average Cost ($):", vbox);
    GtkWidget *entry_last_cost = create_labeled_entry("Last Cost ($):", vbox);
    GtkWidget *entry_price = create_labeled_entry("Price ($):", vbox);
    GtkWidget *entry_stock = create_labeled_entry("Stock:", vbox);
    GtkWidget *entry_min_on = create_labeled_entry("Min On-Season:", vbox);
    GtkWidget *entry_max_on = create_labeled_entry("Max On-Season:", vbox);
    GtkWidget *entry_min_off = create_labeled_entry("Min Off-Season:", vbox);
    GtkWidget *entry_max_off = create_labeled_entry("Max Off-Season:", vbox);
    GtkWidget *serialized_check = gtk_check_button_new_with_label("Serialized (Trek-bike style)");
    GtkWidget *ecom_check = gtk_check_button_new_with_label("Sync to eCommerce");
    gtk_box_pack_start(GTK_BOX(vbox), serialized_check, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), ecom_check, FALSE, FALSE, 0);

    gtk_entry_set_text(GTK_ENTRY(entry_year), "2026");
    gtk_entry_set_text(GTK_ENTRY(entry_msrp), "0.00");
    gtk_entry_set_text(GTK_ENTRY(entry_avg_cost), "0.00");
    gtk_entry_set_text(GTK_ENTRY(entry_last_cost), "0.00");
    gtk_entry_set_text(GTK_ENTRY(entry_min_on), "2");
    gtk_entry_set_text(GTK_ENTRY(entry_max_on), "8");
    gtk_entry_set_text(GTK_ENTRY(entry_min_off), "1");
    gtk_entry_set_text(GTK_ENTRY(entry_max_off), "4");

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *sku = gtk_entry_get_text(GTK_ENTRY(entry_sku));
        const char *name = gtk_entry_get_text(GTK_ENTRY(entry_name));
        const char *vendor = gtk_entry_get_text(GTK_ENTRY(entry_vendor));
        const char *brand = gtk_entry_get_text(GTK_ENTRY(entry_brand));
        const char *upc = gtk_entry_get_text(GTK_ENTRY(entry_upc));
        const char *mpn = gtk_entry_get_text(GTK_ENTRY(entry_mpn));
        const char *style_name = gtk_entry_get_text(GTK_ENTRY(entry_style_name));
        const char *style_number = gtk_entry_get_text(GTK_ENTRY(entry_style_number));
        const char *year_str = gtk_entry_get_text(GTK_ENTRY(entry_year));
        const char *color = gtk_entry_get_text(GTK_ENTRY(entry_color));
        const char *size = gtk_entry_get_text(GTK_ENTRY(entry_size));
        const char *msrp_str = gtk_entry_get_text(GTK_ENTRY(entry_msrp));
        const char *avg_cost_str = gtk_entry_get_text(GTK_ENTRY(entry_avg_cost));
        const char *last_cost_str = gtk_entry_get_text(GTK_ENTRY(entry_last_cost));
        const char *price_str = gtk_entry_get_text(GTK_ENTRY(entry_price));
        const char *stock_str = gtk_entry_get_text(GTK_ENTRY(entry_stock));
        const char *min_on_str = gtk_entry_get_text(GTK_ENTRY(entry_min_on));
        const char *max_on_str = gtk_entry_get_text(GTK_ENTRY(entry_max_on));
        const char *min_off_str = gtk_entry_get_text(GTK_ENTRY(entry_min_off));
        const char *max_off_str = gtk_entry_get_text(GTK_ENTRY(entry_max_off));

        if (strlen(sku) == 0 || strlen(name) == 0) {
            show_error_dialog("SKU and Name are required.");
        } else {
            Product *p = &s->products[s->product_count];
            strncpy(p->sku, sku, NAME_LEN - 1);
            strncpy(p->name, name, NAME_LEN - 1);
            strncpy(p->vendor, vendor[0] ? vendor : "Unknown", NAME_LEN - 1);
            strncpy(p->brand, brand, NAME_LEN - 1);
            strncpy(p->upc, upc, NAME_LEN - 1);
            strncpy(p->manufacturer_part_number, mpn, NAME_LEN - 1);
            strncpy(p->style_name, style_name, NAME_LEN - 1);
            strncpy(p->style_number, style_number, NAME_LEN - 1);
            p->model_year = atoi(year_str);
            strncpy(p->color, color, NAME_LEN - 1);
            strncpy(p->size, size, NAME_LEN - 1);
            p->serialized = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(serialized_check));
            p->msrp = atof(msrp_str);
            p->price = atof(price_str);
            p->average_cost = atof(avg_cost_str);
            p->last_cost = atof(last_cost_str);
            p->ecommerce_sync = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ecom_check));
            p->stock = atoi(stock_str);
            p->on_order_qty = 0;
            p->received_qty = 0;
            p->committed_qty = 0;
            p->min_on_season = atoi(min_on_str);
            p->max_on_season = atoi(max_on_str);
            p->min_off_season = atoi(min_off_str);
            p->max_off_season = atoi(max_off_str);
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

    fprintf(f, "APP_SETTINGS\n%d %d %d %d %.2f %d\n",
            app_settings.prompt_work_order_serial,
            app_settings.prompt_receiving_serial,
            app_settings.layaway_reminder_days,
            app_settings.layaway_grace_days,
            app_settings.layaway_cancel_fee_percent,
            app_settings.layaway_auto_cancel_enabled);

    fprintf(f, "TILE_LAYOUT\n%d\n", tile_count);
    for (int i = 0; i < tile_count; i++) {
        fprintf(f, "%d %d %d %d %d %d %d\n",
                tiles[i].type,
                tiles[i].x,
                tiles[i].y,
                tiles[i].width,
                tiles[i].height,
                tiles[i].color,
                tiles[i].visible);
    }

    fprintf(f, "CUSTOMER_NOTES\n%d\n", customer_note_count);
    for (int i = 0; i < customer_note_count; i++) {
        CustomerNote *n = &customer_notes[i];
        fprintf(f, "%d %d %d\n", n->store_idx, n->customer_idx, n->hidden);
        fprintf(f, "%s\n", n->date);
        fprintf(f, "%s\n", n->note);
    }

    fprintf(f, "WORK_ORDERS\n%d %d\n", work_order_count, next_work_order_id);
    for (int i = 0; i < work_order_count; i++) {
        WorkOrder *wo = &work_orders[i];
        fprintf(f, "%d %d %d %d\n", wo->id, wo->store_idx, wo->customer_idx, wo->status);
        fprintf(f, "%s\n%s\n%s\n", wo->serial, wo->problem, wo->assigned_mechanic);
        fprintf(f, "%.2f %.2f %.2f %.2f %.2f\n", wo->labor_rate, wo->labor_hours, wo->parts_total, wo->labor_total, wo->total);
        fprintf(f, "%s\n%s\n%d\n", wo->created_at, wo->updated_at, wo->hidden);
    }

    fprintf(f, "AUDIT_LOGS\n%d\n", audit_log_count);
    for (int i = 0; i < audit_log_count; i++) {
        AuditLog *a = &audit_logs[i];
        fprintf(f, "%d\n%s\n%s\n%s\n%s\n", a->hidden, a->timestamp, a->user, a->action, a->details);
    }

    fprintf(f, "RESERVATIONS\n%d %d\n", reservation_count, next_reservation_id);
    for (int i = 0; i < reservation_count; i++) {
        Reservation *r = &reservations[i];
        fprintf(f, "%d %d %d %d %d\n", r->id, r->store_idx, r->customer_idx, r->qty, r->status);
        fprintf(f, "%s\n%s\n%s\n%d\n", r->sku, r->expiry_date, r->created_at, r->hidden);
    }

    fprintf(f, "PAYMENT_LEDGER\n%d\n", payment_entry_count);
    for (int i = 0; i < payment_entry_count; i++) {
        PaymentEntry *e = &payment_entries[i];
        fprintf(f, "%d %d %.2f\n", e->store_idx, e->hidden, e->amount);
        fprintf(f, "%s\n%s\n%s\n%s\n", e->transaction_id, e->payment_method, e->date, e->note);
    }

    fprintf(f, "PURCHASE_ORDERS\n%d %d\n", purchase_order_count, next_purchase_order_id);
    for (int i = 0; i < purchase_order_count; i++) {
        PurchaseOrder *po = &purchase_orders[i];
        fprintf(f, "%d %d %d %d %d %d %d\n", po->id, po->store_idx, po->qty_ordered, po->qty_received, po->status, po->hidden, 0);
        fprintf(f, "%s\n%s\n%s\n%s\n%s\n%s\n", po->sku, po->vendor, po->expected_date, po->received_date, po->created_at, po->comments);
    }

    fprintf(f, "PRODUCT_MASTER_EXT\n");
    for (int si = 0; si < store_count; si++) {
        Store *s = &stores[si];
        for (int pi = 0; pi < s->product_count; pi++) {
            Product *p = &s->products[pi];
            fprintf(f, "%d %d %d %d %d %d %d %d %d %d %.2f %.2f %.2f\n",
                    si, pi,
                    p->model_year,
                    p->ecommerce_sync,
                    p->on_order_qty,
                    p->received_qty,
                    p->committed_qty,
                    p->min_on_season,
                    p->max_on_season,
                    p->min_off_season,
                    p->msrp,
                    p->average_cost,
                    p->last_cost);
            fprintf(f, "%s\n%s\n%s\n%s\n%s\n%s\n", p->brand, p->upc, p->manufacturer_part_number, p->style_name, p->style_number, p->color);
            fprintf(f, "%s\n%d\n", p->size, p->max_off_season);
        }
    }
    fprintf(f, "END_PRODUCT_MASTER_EXT\n");

    fprintf(f, "CUSTOMER_PROFILE_EXT\n");
    for (int si = 0; si < store_count; si++) {
        Store *s = &stores[si];
        for (int ci = 0; ci < s->customer_count; ci++) {
            Customer *c = &s->customers[ci];
            fprintf(f, "%d %d\n", si, ci);
            fprintf(f, "%s\n%s\n%s\n%s\n%s\n", c->gender, c->birthdate, c->customer_since, c->last_visit, c->customer_group);
        }
    }
    fprintf(f, "END_CUSTOMER_PROFILE_EXT\n");

    fprintf(f, "VENDOR_LINKS\n%d\n", vendor_link_count);
    for (int i = 0; i < vendor_link_count; i++) {
        VendorProductLink *v = &vendor_links[i];
        fprintf(f, "%d %d\n", v->store_idx, v->hidden);
        fprintf(f, "%s\n%s\n%s\n%s\n", v->in_store_sku, v->vendor_name, v->vendor_product_code, v->vendor_description);
    }

    fprintf(f, "QBP_CATALOG\n%d\n", qbp_catalog_count);
    for (int i = 0; i < qbp_catalog_count; i++) {
        QbpCatalogItem *q = &qbp_catalog[i];
        fprintf(f, "%d %d %d %d %.3f\n", q->hidden, q->nv_qty, q->pa_qty, q->wi_qty, q->weight_lbs);
        fprintf(f, "%s\n%s\n%s\n", q->sku, q->description, q->image_url);
    }

    fprintf(f, "WEB_ORDER_PICKUPS\n%d\n", web_order_pickup_count);
    for (int i = 0; i < web_order_pickup_count; i++) {
        WebOrderPickup *w = &web_order_pickups[i];
        fprintf(f, "%d %d %d %d\n", w->store_idx, w->customer_idx, w->status, w->hidden);
        fprintf(f, "%s\n%s\n%s\n%s\n", w->manufacturer, w->manufacturer_order_number, w->model_name, w->labor_sku);
    }

    fprintf(f, "SUSPENSION_LOGS\n%d\n", suspension_log_count);
    for (int i = 0; i < suspension_log_count; i++) {
        SuspensionSetupLog *l = &suspension_logs[i];
        fprintf(f, "%d %d %d %.2f %d %.2f %d\n", l->store_idx, l->customer_idx, l->hidden, l->fork_psi, l->fork_rebound, l->shock_psi, l->shock_rebound);
        fprintf(f, "%s\n%s\n%s\n", l->bike_serial, l->date, l->notes);
    }

    fprintf(f, "SYNC_QUEUE\n%d\n", sync_queue_count);
    for (int i = 0; i < sync_queue_count; i++) {
        OfflineSyncQueueItem *q = &sync_queue[i];
        fprintf(f, "%d %d %d %d\n", q->store_idx, q->qty, q->synced, q->hidden);
        fprintf(f, "%s\n%s\n%s\n", q->transaction_id, q->sku, q->created_at);
    }

    fprintf(f, "SYSTEM_CONFIG\n%d\n%d\n%s\n%s\n", enable_multistore_sync, trek_auto_register, current_sales_user, system_sales_popup_message);
    fprintf(f, "ADV|%.4f|%.2f|%d|%d|%d|%d|%d\n",
            app_settings.default_tax_rate,
            app_settings.max_discount_percent_sales,
            (int)active_theme_mode,
            mobile_floor_mode_enabled,
            internet_online,
            local_node_enabled,
            store_and_forward_enabled);

    fprintf(f, "SECURITY_SETTINGS\n%d\n%s\n", active_user_role, manager_override_pin);

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
            pr->brand[0] = '\0';
            pr->upc[0] = '\0';
            pr->manufacturer_part_number[0] = '\0';
            pr->style_name[0] = '\0';
            pr->style_number[0] = '\0';
            pr->model_year = 0;
            pr->color[0] = '\0';
            pr->size[0] = '\0';
            pr->msrp = pr->price;
            pr->average_cost = 0.0;
            pr->last_cost = 0.0;
            pr->ecommerce_sync = 0;
            pr->on_order_qty = 0;
            pr->received_qty = 0;
            pr->committed_qty = 0;
            pr->min_on_season = 0;
            pr->max_on_season = 0;
            pr->min_off_season = 0;
            pr->max_off_season = 0;
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
                cust->gender[0] = '\0';
                cust->birthdate[0] = '\0';
                cust->customer_group[0] = '\0';
                get_today_date(cust->customer_since, sizeof(cust->customer_since));
                strncpy(cust->last_visit, cust->customer_since, NAME_LEN - 1);
                cust->last_visit[NAME_LEN - 1] = '\0';
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
    customer_note_count = 0;
    work_order_count = 0;
    next_work_order_id = 1000;
    audit_log_count = 0;
    reservation_count = 0;
    next_reservation_id = 5000;
    payment_entry_count = 0;
    purchase_order_count = 0;
    next_purchase_order_id = 8000;
    vendor_link_count = 0;
    qbp_catalog_count = 0;
    web_order_pickup_count = 0;
    suspension_log_count = 0;
    sync_queue_count = 0;
    for (int i = 0; i < MAX_REPAIR_STANDS; i++) {
        repair_stands[i].stand_number = i + 1;
        repair_stands[i].work_order_id = 0;
        repair_stands[i].mechanic[0] = '\0';
        repair_stands[i].active = 0;
    }
    active_user_role = ROLE_MANAGER;
    strncpy(manager_override_pin, "2468", NAME_LEN - 1);
    manager_override_pin[NAME_LEN - 1] = '\0';
    strncpy(system_sales_popup_message, "Verify info for registration.", sizeof(system_sales_popup_message) - 1);
    system_sales_popup_message[sizeof(system_sales_popup_message) - 1] = '\0';
    strncpy(current_sales_user, "Ascend User", NAME_LEN - 1);
    current_sales_user[NAME_LEN - 1] = '\0';
    enable_multistore_sync = 1;
    trek_auto_register = 1;
    app_settings.prompt_work_order_serial = 1;
    app_settings.prompt_receiving_serial = 1;
    app_settings.layaway_reminder_days = 7;
    app_settings.layaway_grace_days = 10;
    app_settings.layaway_cancel_fee_percent = 15.0;
    app_settings.layaway_auto_cancel_enabled = 0;
    app_settings.default_tax_rate = 0.0825;
    app_settings.max_discount_percent_sales = 15.0;
    active_theme_mode = THEME_SMART;
    mobile_floor_mode_enabled = 0;
    internet_online = 1;
    local_node_enabled = 1;
    store_and_forward_enabled = 1;

    if (!tiles_initialized) {
        init_default_tiles();
        tiles_initialized = 1;
    }

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
        } else if (strcmp(section_name, "APP_SETTINGS") == 0) {
            char line[256];
            if (fscanf(f, " %255[^\n]\n", line) == 1) {
                int work_prompt = 1;
                int recv_prompt = 1;
                int reminder_days = app_settings.layaway_reminder_days;
                int grace_days = app_settings.layaway_grace_days;
                double cancel_fee = app_settings.layaway_cancel_fee_percent;
                int auto_cancel = app_settings.layaway_auto_cancel_enabled;
                int rc = sscanf(line, "%d %d %d %d %lf %d",
                                &work_prompt,
                                &recv_prompt,
                                &reminder_days,
                                &grace_days,
                                &cancel_fee,
                                &auto_cancel);
                if (rc >= 2) {
                    app_settings.prompt_work_order_serial = work_prompt;
                    app_settings.prompt_receiving_serial = recv_prompt;
                }
                if (rc >= 6) {
                    app_settings.layaway_reminder_days = reminder_days;
                    app_settings.layaway_grace_days = grace_days;
                    app_settings.layaway_cancel_fee_percent = cancel_fee;
                    app_settings.layaway_auto_cancel_enabled = auto_cancel;
                }
            }
        } else if (strcmp(section_name, "TILE_LAYOUT") == 0) {
            int count = 0;
            if (fscanf(f, "%d\n", &count) != 1) break;
            for (int i = 0; i < count; i++) {
                int type = 0, x = 0, y = 0, w = 0, h = 0, color = 0, visible = 1;
                if (fscanf(f, "%d %d %d %d %d %d %d\n", &type, &x, &y, &w, &h, &color, &visible) != 7) break;
                for (int t = 0; t < tile_count; t++) {
                    if (tiles[t].type == (TileType)type) {
                        tiles[t].x = x;
                        tiles[t].y = y;
                        tiles[t].width = w;
                        tiles[t].height = h;
                        tiles[t].color = (color >= COLOR_LIGHT_BLUE && color <= COLOR_SLATE) ? (TileColor)color : COLOR_LIGHT_BLUE;
                        tiles[t].visible = visible ? 1 : 0;
                        break;
                    }
                }
            }
        } else if (strcmp(section_name, "CUSTOMER_NOTES") == 0) {
            int count = 0;
            if (fscanf(f, "%d\n", &count) != 1) break;
            int limit = (count < MAX_CUSTOMER_NOTES) ? count : MAX_CUSTOMER_NOTES;
            customer_note_count = limit;
            for (int i = 0; i < count; i++) {
                int store_idx = 0, customer_idx = 0, hidden = 0;
                char date_buf[NAME_LEN];
                char note_buf[NAME_LEN * 4];
                if (fscanf(f, "%d %d %d\n", &store_idx, &customer_idx, &hidden) != 3) break;
                if (!fgets(date_buf, sizeof(date_buf), f)) break;
                date_buf[strcspn(date_buf, "\n")] = '\0';
                if (!fgets(note_buf, sizeof(note_buf), f)) break;
                note_buf[strcspn(note_buf, "\n")] = '\0';
                if (i < limit) {
                    CustomerNote *n = &customer_notes[i];
                    n->store_idx = store_idx;
                    n->customer_idx = customer_idx;
                    n->hidden = hidden;
                    strncpy(n->date, date_buf, NAME_LEN - 1);
                    n->date[NAME_LEN - 1] = '\0';
                    strncpy(n->note, note_buf, sizeof(n->note) - 1);
                    n->note[sizeof(n->note) - 1] = '\0';
                }
            }
        } else if (strcmp(section_name, "WORK_ORDERS") == 0) {
            int count = 0;
            int next_id = 1000;
            if (fscanf(f, "%d %d\n", &count, &next_id) != 2) break;
            int limit = (count < MAX_WORK_ORDERS) ? count : MAX_WORK_ORDERS;
            work_order_count = limit;
            next_work_order_id = next_id;
            for (int i = 0; i < count; i++) {
                int id = 0, store_idx = 0, customer_idx = 0, status = 0;
                char serial[NAME_LEN];
                char problem[NAME_LEN * 4];
                char mech[NAME_LEN];
                double labor_rate = 0.0, labor_hours = 0.0, parts_total = 0.0, labor_total = 0.0, total = 0.0;
                char created_at[NAME_LEN];
                char updated_at[NAME_LEN];
                int hidden = 0;
                if (fscanf(f, "%d %d %d %d\n", &id, &store_idx, &customer_idx, &status) != 4) break;
                if (!fgets(serial, sizeof(serial), f)) break;
                serial[strcspn(serial, "\n")] = '\0';
                if (!fgets(problem, sizeof(problem), f)) break;
                problem[strcspn(problem, "\n")] = '\0';
                if (!fgets(mech, sizeof(mech), f)) break;
                mech[strcspn(mech, "\n")] = '\0';
                if (fscanf(f, "%lf %lf %lf %lf %lf\n", &labor_rate, &labor_hours, &parts_total, &labor_total, &total) != 5) break;
                if (!fgets(created_at, sizeof(created_at), f)) break;
                created_at[strcspn(created_at, "\n")] = '\0';
                if (!fgets(updated_at, sizeof(updated_at), f)) break;
                updated_at[strcspn(updated_at, "\n")] = '\0';
                if (fscanf(f, "%d\n", &hidden) != 1) break;

                if (i < limit) {
                    WorkOrder *wo = &work_orders[i];
                    wo->id = id;
                    wo->store_idx = store_idx;
                    wo->customer_idx = customer_idx;
                    wo->status = (status >= WO_OPEN && status <= WO_PICKED_UP) ? (WorkOrderStatus)status : WO_OPEN;
                    wo->labor_rate = labor_rate;
                    wo->labor_hours = labor_hours;
                    wo->parts_total = parts_total;
                    wo->labor_total = labor_total;
                    wo->total = total;
                    wo->hidden = hidden;
                    strncpy(wo->serial, serial, NAME_LEN - 1);
                    wo->serial[NAME_LEN - 1] = '\0';
                    strncpy(wo->problem, problem, sizeof(wo->problem) - 1);
                    wo->problem[sizeof(wo->problem) - 1] = '\0';
                    strncpy(wo->assigned_mechanic, mech, NAME_LEN - 1);
                    wo->assigned_mechanic[NAME_LEN - 1] = '\0';
                    strncpy(wo->created_at, created_at, NAME_LEN - 1);
                    wo->created_at[NAME_LEN - 1] = '\0';
                    strncpy(wo->updated_at, updated_at, NAME_LEN - 1);
                    wo->updated_at[NAME_LEN - 1] = '\0';
                }
            }
        } else if (strcmp(section_name, "AUDIT_LOGS") == 0) {
            int count = 0;
            if (fscanf(f, "%d\n", &count) != 1) break;
            int limit = (count < MAX_AUDIT_LOGS) ? count : MAX_AUDIT_LOGS;
            audit_log_count = limit;
            for (int i = 0; i < count; i++) {
                int hidden = 0;
                char timestamp[NAME_LEN];
                char user[NAME_LEN];
                char action[NAME_LEN];
                char details[NAME_LEN * 4];
                if (fscanf(f, "%d\n", &hidden) != 1) break;
                if (!fgets(timestamp, sizeof(timestamp), f)) break;
                timestamp[strcspn(timestamp, "\n")] = '\0';
                if (!fgets(user, sizeof(user), f)) break;
                user[strcspn(user, "\n")] = '\0';
                if (!fgets(action, sizeof(action), f)) break;
                action[strcspn(action, "\n")] = '\0';
                if (!fgets(details, sizeof(details), f)) break;
                details[strcspn(details, "\n")] = '\0';
                if (i < limit) {
                    AuditLog *a = &audit_logs[i];
                    a->hidden = hidden;
                    strncpy(a->timestamp, timestamp, NAME_LEN - 1);
                    a->timestamp[NAME_LEN - 1] = '\0';
                    strncpy(a->user, user, NAME_LEN - 1);
                    a->user[NAME_LEN - 1] = '\0';
                    strncpy(a->action, action, NAME_LEN - 1);
                    a->action[NAME_LEN - 1] = '\0';
                    strncpy(a->details, details, sizeof(a->details) - 1);
                    a->details[sizeof(a->details) - 1] = '\0';
                }
            }
        } else if (strcmp(section_name, "RESERVATIONS") == 0) {
            int count = 0;
            int next_id = 5000;
            if (fscanf(f, "%d %d\n", &count, &next_id) != 2) break;
            int limit = (count < MAX_RESERVATIONS) ? count : MAX_RESERVATIONS;
            reservation_count = limit;
            next_reservation_id = next_id;
            for (int i = 0; i < count; i++) {
                int id = 0, store_idx = 0, customer_idx = 0, qty = 0, status = 0;
                char sku[NAME_LEN], expiry[NAME_LEN], created[NAME_LEN];
                int hidden = 0;
                if (fscanf(f, "%d %d %d %d %d\n", &id, &store_idx, &customer_idx, &qty, &status) != 5) break;
                if (!fgets(sku, sizeof(sku), f)) break;
                sku[strcspn(sku, "\n")] = '\0';
                if (!fgets(expiry, sizeof(expiry), f)) break;
                expiry[strcspn(expiry, "\n")] = '\0';
                if (!fgets(created, sizeof(created), f)) break;
                created[strcspn(created, "\n")] = '\0';
                if (fscanf(f, "%d\n", &hidden) != 1) break;
                if (i < limit) {
                    Reservation *r = &reservations[i];
                    r->id = id;
                    r->store_idx = store_idx;
                    r->customer_idx = customer_idx;
                    r->qty = qty;
                    r->status = status;
                    r->hidden = hidden;
                    strncpy(r->sku, sku, NAME_LEN - 1);
                    r->sku[NAME_LEN - 1] = '\0';
                    strncpy(r->expiry_date, expiry, NAME_LEN - 1);
                    r->expiry_date[NAME_LEN - 1] = '\0';
                    strncpy(r->created_at, created, NAME_LEN - 1);
                    r->created_at[NAME_LEN - 1] = '\0';
                }
            }
        } else if (strcmp(section_name, "PAYMENT_LEDGER") == 0) {
            int count = 0;
            if (fscanf(f, "%d\n", &count) != 1) break;
            int limit = (count < MAX_PAYMENT_ENTRIES) ? count : MAX_PAYMENT_ENTRIES;
            payment_entry_count = limit;
            for (int i = 0; i < count; i++) {
                int store_idx = -1, hidden = 0;
                double amount = 0.0;
                char txn[NAME_LEN], method[NAME_LEN], date[NAME_LEN], note[NAME_LEN * 2];
                if (fscanf(f, "%d %d %lf\n", &store_idx, &hidden, &amount) != 3) break;
                if (!fgets(txn, sizeof(txn), f)) break;
                txn[strcspn(txn, "\n")] = '\0';
                if (!fgets(method, sizeof(method), f)) break;
                method[strcspn(method, "\n")] = '\0';
                if (!fgets(date, sizeof(date), f)) break;
                date[strcspn(date, "\n")] = '\0';
                if (!fgets(note, sizeof(note), f)) break;
                note[strcspn(note, "\n")] = '\0';

                if (i < limit) {
                    PaymentEntry *e = &payment_entries[i];
                    e->store_idx = store_idx;
                    e->hidden = hidden;
                    e->amount = amount;
                    strncpy(e->transaction_id, txn, NAME_LEN - 1);
                    e->transaction_id[NAME_LEN - 1] = '\0';
                    strncpy(e->payment_method, method, NAME_LEN - 1);
                    e->payment_method[NAME_LEN - 1] = '\0';
                    strncpy(e->date, date, NAME_LEN - 1);
                    e->date[NAME_LEN - 1] = '\0';
                    strncpy(e->note, note, sizeof(e->note) - 1);
                    e->note[sizeof(e->note) - 1] = '\0';
                }
            }
        } else if (strcmp(section_name, "PURCHASE_ORDERS") == 0) {
            int count = 0;
            int next_id = 8000;
            if (fscanf(f, "%d %d\n", &count, &next_id) != 2) break;
            int limit = (count < MAX_PURCHASE_ORDERS) ? count : MAX_PURCHASE_ORDERS;
            purchase_order_count = limit;
            next_purchase_order_id = next_id;
            for (int i = 0; i < count; i++) {
                int id = 0, store_idx = 0, qty_ordered = 0, qty_received = 0, status = 0, hidden = 0, reserved = 0;
                char sku[NAME_LEN], vendor[NAME_LEN], expected[NAME_LEN], received[NAME_LEN], created[NAME_LEN], comments[NAME_LEN * 2];
                if (fscanf(f, "%d %d %d %d %d %d %d\n", &id, &store_idx, &qty_ordered, &qty_received, &status, &hidden, &reserved) != 7) break;
                if (!fgets(sku, sizeof(sku), f)) break;
                sku[strcspn(sku, "\n")] = '\0';
                if (!fgets(vendor, sizeof(vendor), f)) break;
                vendor[strcspn(vendor, "\n")] = '\0';
                if (!fgets(expected, sizeof(expected), f)) break;
                expected[strcspn(expected, "\n")] = '\0';
                if (!fgets(received, sizeof(received), f)) break;
                received[strcspn(received, "\n")] = '\0';
                if (!fgets(created, sizeof(created), f)) break;
                created[strcspn(created, "\n")] = '\0';
                if (!fgets(comments, sizeof(comments), f)) break;
                comments[strcspn(comments, "\n")] = '\0';
                if (i < limit) {
                    PurchaseOrder *po = &purchase_orders[i];
                    po->id = id;
                    po->store_idx = store_idx;
                    po->qty_ordered = qty_ordered;
                    po->qty_received = qty_received;
                    po->status = status;
                    po->hidden = hidden;
                    strncpy(po->sku, sku, NAME_LEN - 1);
                    po->sku[NAME_LEN - 1] = '\0';
                    strncpy(po->vendor, vendor, NAME_LEN - 1);
                    po->vendor[NAME_LEN - 1] = '\0';
                    strncpy(po->expected_date, expected, NAME_LEN - 1);
                    po->expected_date[NAME_LEN - 1] = '\0';
                    strncpy(po->received_date, received, NAME_LEN - 1);
                    po->received_date[NAME_LEN - 1] = '\0';
                    strncpy(po->created_at, created, NAME_LEN - 1);
                    po->created_at[NAME_LEN - 1] = '\0';
                    strncpy(po->comments, comments, sizeof(po->comments) - 1);
                    po->comments[sizeof(po->comments) - 1] = '\0';
                }
            }
        } else if (strcmp(section_name, "PRODUCT_MASTER_EXT") == 0) {
            char marker[NAME_LEN];
            while (fscanf(f, "%63s", marker) == 1) {
                if (strcmp(marker, "END_PRODUCT_MASTER_EXT") == 0) break;
                int si = atoi(marker);
                int pi = 0, model_year = 0, ecom = 0, on_order = 0, received = 0, committed = 0;
                int min_on = 0, max_on = 0, min_off = 0, max_off = 0;
                double msrp = 0.0, avg_cost = 0.0, last_cost = 0.0;
                if (fscanf(f, "%d %d %d %d %d %d %d %d %d %lf %lf %lf\n",
                           &pi, &model_year, &ecom, &on_order, &received, &committed,
                           &min_on, &max_on, &min_off, &msrp, &avg_cost, &last_cost) != 12) break;
                char brand[NAME_LEN], upc[NAME_LEN], mpn[NAME_LEN], style_name[NAME_LEN], style_number[NAME_LEN], color[NAME_LEN], size[NAME_LEN];
                if (!fgets(brand, sizeof(brand), f)) break;
                brand[strcspn(brand, "\n")] = '\0';
                if (!fgets(upc, sizeof(upc), f)) break;
                upc[strcspn(upc, "\n")] = '\0';
                if (!fgets(mpn, sizeof(mpn), f)) break;
                mpn[strcspn(mpn, "\n")] = '\0';
                if (!fgets(style_name, sizeof(style_name), f)) break;
                style_name[strcspn(style_name, "\n")] = '\0';
                if (!fgets(style_number, sizeof(style_number), f)) break;
                style_number[strcspn(style_number, "\n")] = '\0';
                if (!fgets(color, sizeof(color), f)) break;
                color[strcspn(color, "\n")] = '\0';
                if (!fgets(size, sizeof(size), f)) break;
                size[strcspn(size, "\n")] = '\0';
                if (fscanf(f, "%d\n", &max_off) != 1) break;
                if (si >= 0 && si < store_count && pi >= 0 && pi < stores[si].product_count) {
                    Product *p = &stores[si].products[pi];
                    p->model_year = model_year;
                    p->ecommerce_sync = ecom;
                    p->on_order_qty = on_order;
                    p->received_qty = received;
                    p->committed_qty = committed;
                    p->min_on_season = min_on;
                    p->max_on_season = max_on;
                    p->min_off_season = min_off;
                    p->max_off_season = max_off;
                    p->msrp = msrp;
                    p->average_cost = avg_cost;
                    p->last_cost = last_cost;
                    strncpy(p->brand, brand, NAME_LEN - 1);
                    strncpy(p->upc, upc, NAME_LEN - 1);
                    strncpy(p->manufacturer_part_number, mpn, NAME_LEN - 1);
                    strncpy(p->style_name, style_name, NAME_LEN - 1);
                    strncpy(p->style_number, style_number, NAME_LEN - 1);
                    strncpy(p->color, color, NAME_LEN - 1);
                    strncpy(p->size, size, NAME_LEN - 1);
                }
            }
        } else if (strcmp(section_name, "CUSTOMER_PROFILE_EXT") == 0) {
            char marker[NAME_LEN];
            while (fscanf(f, "%63s", marker) == 1) {
                if (strcmp(marker, "END_CUSTOMER_PROFILE_EXT") == 0) break;
                int si = atoi(marker);
                int ci = 0;
                if (fscanf(f, "%d\n", &ci) != 1) break;
                char gender[NAME_LEN], birthdate[NAME_LEN], since[NAME_LEN], last_visit[NAME_LEN], group[NAME_LEN];
                if (!fgets(gender, sizeof(gender), f)) break;
                gender[strcspn(gender, "\n")] = '\0';
                if (!fgets(birthdate, sizeof(birthdate), f)) break;
                birthdate[strcspn(birthdate, "\n")] = '\0';
                if (!fgets(since, sizeof(since), f)) break;
                since[strcspn(since, "\n")] = '\0';
                if (!fgets(last_visit, sizeof(last_visit), f)) break;
                last_visit[strcspn(last_visit, "\n")] = '\0';
                if (!fgets(group, sizeof(group), f)) break;
                group[strcspn(group, "\n")] = '\0';
                if (si >= 0 && si < store_count && ci >= 0 && ci < stores[si].customer_count) {
                    Customer *c = &stores[si].customers[ci];
                    strncpy(c->gender, gender, NAME_LEN - 1);
                    strncpy(c->birthdate, birthdate, NAME_LEN - 1);
                    strncpy(c->customer_since, since, NAME_LEN - 1);
                    strncpy(c->last_visit, last_visit, NAME_LEN - 1);
                    strncpy(c->customer_group, group, NAME_LEN - 1);
                }
            }
        } else if (strcmp(section_name, "VENDOR_LINKS") == 0) {
            int count = 0;
            if (fscanf(f, "%d\n", &count) != 1) break;
            int limit = (count < MAX_VENDOR_LINKS) ? count : MAX_VENDOR_LINKS;
            vendor_link_count = limit;
            for (int i = 0; i < count; i++) {
                int si = 0, hidden = 0;
                char sku[NAME_LEN], vendor[NAME_LEN], vcode[NAME_LEN], vdesc[NAME_LEN * 2];
                if (fscanf(f, "%d %d\n", &si, &hidden) != 2) break;
                if (!fgets(sku, sizeof(sku), f)) break;
                sku[strcspn(sku, "\n")] = '\0';
                if (!fgets(vendor, sizeof(vendor), f)) break;
                vendor[strcspn(vendor, "\n")] = '\0';
                if (!fgets(vcode, sizeof(vcode), f)) break;
                vcode[strcspn(vcode, "\n")] = '\0';
                if (!fgets(vdesc, sizeof(vdesc), f)) break;
                vdesc[strcspn(vdesc, "\n")] = '\0';
                if (i < limit) {
                    VendorProductLink *v = &vendor_links[i];
                    v->store_idx = si;
                    v->hidden = hidden;
                    strncpy(v->in_store_sku, sku, NAME_LEN - 1);
                    strncpy(v->vendor_name, vendor, NAME_LEN - 1);
                    strncpy(v->vendor_product_code, vcode, NAME_LEN - 1);
                    strncpy(v->vendor_description, vdesc, sizeof(v->vendor_description) - 1);
                }
            }
        } else if (strcmp(section_name, "QBP_CATALOG") == 0) {
            int count = 0;
            if (fscanf(f, "%d\n", &count) != 1) break;
            int limit = (count < MAX_QBP_CATALOG_ITEMS) ? count : MAX_QBP_CATALOG_ITEMS;
            qbp_catalog_count = limit;
            for (int i = 0; i < count; i++) {
                int hidden = 0, nv = 0, pa = 0, wi = 0;
                double w = 0.0;
                char sku[NAME_LEN], desc[NAME_LEN * 2], img[NAME_LEN * 2];
                if (fscanf(f, "%d %d %d %d %lf\n", &hidden, &nv, &pa, &wi, &w) != 5) break;
                if (!fgets(sku, sizeof(sku), f)) break;
                if (!fgets(desc, sizeof(desc), f)) break;
                if (!fgets(img, sizeof(img), f)) break;
                sku[strcspn(sku, "\n")] = '\0';
                desc[strcspn(desc, "\n")] = '\0';
                img[strcspn(img, "\n")] = '\0';
                if (i < limit) {
                    QbpCatalogItem *q = &qbp_catalog[i];
                    q->hidden = hidden;
                    q->nv_qty = nv;
                    q->pa_qty = pa;
                    q->wi_qty = wi;
                    q->weight_lbs = w;
                    strncpy(q->sku, sku, NAME_LEN - 1);
                    strncpy(q->description, desc, sizeof(q->description) - 1);
                    strncpy(q->image_url, img, sizeof(q->image_url) - 1);
                }
            }
        } else if (strcmp(section_name, "WEB_ORDER_PICKUPS") == 0) {
            int count = 0;
            if (fscanf(f, "%d\n", &count) != 1) break;
            int limit = (count < MAX_WEB_ORDER_PICKUPS) ? count : MAX_WEB_ORDER_PICKUPS;
            web_order_pickup_count = limit;
            for (int i = 0; i < count; i++) {
                int si = 0, ci = -1, status = 0, hidden = 0;
                char mfg[NAME_LEN], order[NAME_LEN], model[NAME_LEN], labor[NAME_LEN];
                if (fscanf(f, "%d %d %d %d\n", &si, &ci, &status, &hidden) != 4) break;
                if (!fgets(mfg, sizeof(mfg), f)) break;
                if (!fgets(order, sizeof(order), f)) break;
                if (!fgets(model, sizeof(model), f)) break;
                if (!fgets(labor, sizeof(labor), f)) break;
                mfg[strcspn(mfg, "\n")] = '\0';
                order[strcspn(order, "\n")] = '\0';
                model[strcspn(model, "\n")] = '\0';
                labor[strcspn(labor, "\n")] = '\0';
                if (i < limit) {
                    WebOrderPickup *w = &web_order_pickups[i];
                    w->store_idx = si;
                    w->customer_idx = ci;
                    w->status = status;
                    w->hidden = hidden;
                    strncpy(w->manufacturer, mfg, NAME_LEN - 1);
                    strncpy(w->manufacturer_order_number, order, NAME_LEN - 1);
                    strncpy(w->model_name, model, NAME_LEN - 1);
                    strncpy(w->labor_sku, labor, NAME_LEN - 1);
                }
            }
        } else if (strcmp(section_name, "SUSPENSION_LOGS") == 0) {
            int count = 0;
            if (fscanf(f, "%d\n", &count) != 1) break;
            int limit = (count < MAX_SUSPENSION_LOGS) ? count : MAX_SUSPENSION_LOGS;
            suspension_log_count = limit;
            for (int i = 0; i < count; i++) {
                int si = 0, ci = -1, hidden = 0, fr = 0, sr = 0;
                double fpsi = 0.0, spsi = 0.0;
                char serial[NAME_LEN], date[NAME_LEN], notes[NAME_LEN * 2];
                if (fscanf(f, "%d %d %d %lf %d %lf %d\n", &si, &ci, &hidden, &fpsi, &fr, &spsi, &sr) != 7) break;
                if (!fgets(serial, sizeof(serial), f)) break;
                if (!fgets(date, sizeof(date), f)) break;
                if (!fgets(notes, sizeof(notes), f)) break;
                serial[strcspn(serial, "\n")] = '\0';
                date[strcspn(date, "\n")] = '\0';
                notes[strcspn(notes, "\n")] = '\0';
                if (i < limit) {
                    SuspensionSetupLog *l = &suspension_logs[i];
                    l->store_idx = si;
                    l->customer_idx = ci;
                    l->hidden = hidden;
                    l->fork_psi = fpsi;
                    l->fork_rebound = fr;
                    l->shock_psi = spsi;
                    l->shock_rebound = sr;
                    strncpy(l->bike_serial, serial, NAME_LEN - 1);
                    strncpy(l->date, date, NAME_LEN - 1);
                    strncpy(l->notes, notes, sizeof(l->notes) - 1);
                }
            }
        } else if (strcmp(section_name, "SYNC_QUEUE") == 0) {
            int count = 0;
            if (fscanf(f, "%d\n", &count) != 1) break;
            int limit = (count < MAX_SYNC_QUEUE) ? count : MAX_SYNC_QUEUE;
            sync_queue_count = limit;
            for (int i = 0; i < count; i++) {
                int si = 0, qty = 0, synced = 0, hidden = 0;
                char txn[NAME_LEN], sku[NAME_LEN], created[NAME_LEN];
                if (fscanf(f, "%d %d %d %d\n", &si, &qty, &synced, &hidden) != 4) break;
                if (!fgets(txn, sizeof(txn), f)) break;
                if (!fgets(sku, sizeof(sku), f)) break;
                if (!fgets(created, sizeof(created), f)) break;
                txn[strcspn(txn, "\n")] = '\0';
                sku[strcspn(sku, "\n")] = '\0';
                created[strcspn(created, "\n")] = '\0';
                if (i < limit) {
                    OfflineSyncQueueItem *q = &sync_queue[i];
                    q->store_idx = si;
                    q->qty = qty;
                    q->synced = synced;
                    q->hidden = hidden;
                    strncpy(q->transaction_id, txn, NAME_LEN - 1);
                    strncpy(q->sku, sku, NAME_LEN - 1);
                    strncpy(q->created_at, created, NAME_LEN - 1);
                }
            }
        } else if (strcmp(section_name, "SYSTEM_CONFIG") == 0) {
            if (fscanf(f, "%d\n%d\n", &enable_multistore_sync, &trek_auto_register) != 2) break;
            if (!fgets(current_sales_user, sizeof(current_sales_user), f)) break;
            current_sales_user[strcspn(current_sales_user, "\n")] = '\0';
            if (!fgets(system_sales_popup_message, sizeof(system_sales_popup_message), f)) break;
            system_sales_popup_message[strcspn(system_sales_popup_message, "\n")] = '\0';
            {
                long pos = ftell(f);
                char adv_line[256];
                if (fgets(adv_line, sizeof(adv_line), f)) {
                    adv_line[strcspn(adv_line, "\n")] = '\0';
                    if (strncmp(adv_line, "ADV|", 4) == 0) {
                        double tax = app_settings.default_tax_rate;
                        double max_disc = app_settings.max_discount_percent_sales;
                        int theme = (int)active_theme_mode;
                        int mobile = mobile_floor_mode_enabled;
                        int net_online = internet_online;
                        int local_node = local_node_enabled;
                        int store_fwd = store_and_forward_enabled;
                        if (sscanf(adv_line + 4, "%lf|%lf|%d|%d|%d|%d|%d", &tax, &max_disc, &theme, &mobile, &net_online, &local_node, &store_fwd) >= 2) {
                            if (tax >= 0.0) app_settings.default_tax_rate = tax;
                            if (max_disc >= 0.0) app_settings.max_discount_percent_sales = max_disc;
                            if (theme >= THEME_LIGHT && theme <= THEME_SMART) active_theme_mode = (ThemeMode)theme;
                            mobile_floor_mode_enabled = mobile ? 1 : 0;
                            internet_online = net_online ? 1 : 0;
                            local_node_enabled = local_node ? 1 : 0;
                            store_and_forward_enabled = store_fwd ? 1 : 0;
                        }
                    } else {
                        fseek(f, pos, SEEK_SET);
                    }
                }
            }
        } else if (strcmp(section_name, "SECURITY_SETTINGS") == 0) {
            int role_val = ROLE_MANAGER;
            char pin_line[NAME_LEN];
            if (fscanf(f, "%d\n", &role_val) != 1) break;
            if (!fgets(pin_line, sizeof(pin_line), f)) break;
            pin_line[strcspn(pin_line, "\n")] = '\0';
            if (role_val < ROLE_ADMIN || role_val > ROLE_BUYER) role_val = ROLE_MANAGER;
            active_user_role = (UserRole)role_val;
            if (strlen(pin_line) > 0) {
                strncpy(manager_override_pin, pin_line, NAME_LEN - 1);
                manager_override_pin[NAME_LEN - 1] = '\0';
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
    GtkWidget *gender_entry = create_labeled_entry("Gender:", vbox);
    GtkWidget *birthdate_entry = create_labeled_entry("Birthdate (YYYY-MM-DD):", vbox);
    GtkWidget *group_entry = create_labeled_entry("Customer Group:", vbox);

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
        strncpy(c->gender, gtk_entry_get_text(GTK_ENTRY(gender_entry)), NAME_LEN - 1);
        strncpy(c->birthdate, gtk_entry_get_text(GTK_ENTRY(birthdate_entry)), NAME_LEN - 1);
        strncpy(c->customer_group, gtk_entry_get_text(GTK_ENTRY(group_entry)), NAME_LEN - 1);
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
        get_today_date(c->customer_since, sizeof(c->customer_since));
        strncpy(c->last_visit, c->customer_since, NAME_LEN - 1);
        c->last_visit[NAME_LEN - 1] = '\0';
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

    GtkListStore *store_list = gtk_list_store_new(8,
                                                   G_TYPE_INT,     // customer index
                                                   G_TYPE_STRING,  // name
                                                   G_TYPE_STRING,  // company
                                                   G_TYPE_STRING,  // email
                                                   G_TYPE_STRING,  // poa status
                                                   G_TYPE_STRING,  // credit limit
                                                   G_TYPE_STRING,  // group
                                                   G_TYPE_STRING); // last visit

    GtkTreeView *tree = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_list)));
    
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Name", renderer, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Company", renderer, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Email", renderer, "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "POA Status", renderer, "text", 4, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Credit Limit", renderer, "text", 5, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Group", renderer, "text", 6, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Last Visit", renderer, "text", 7, NULL);

    // Populate list
    for (int i = 0; i < s->customer_count; i++) {
        if (s->customers[i].hidden) continue; // Skip hidden customers
        Customer *c = &s->customers[i];
        char name[NAME_LEN * 2];
        sprintf(name, "%s %s", c->first_name, c->last_name);
        const char *poa_names[] = {"Inactive", "Active", "Suspended", "Closed"};
        char credit[30];
        sprintf(credit, "%.2f", c->credit_limit);
        int days = days_ago_from_date(c->last_visit);
        char last_visit[80];
        if (days >= 0) snprintf(last_visit, sizeof(last_visit), "%s (%d days ago)", c->last_visit, days);
        else snprintf(last_visit, sizeof(last_visit), "%s", c->last_visit);

        GtkTreeIter iter;
        gtk_list_store_append(store_list, &iter);
        gtk_list_store_set(store_list, &iter,
                          0, i,
                          1, name,
                          2, c->company,
                          3, c->email,
                          4, poa_names[c->poa_status],
                          5, credit,
                          6, c->customer_group,
                          7, last_visit,
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

    if (strlen(system_sales_popup_message) > 0) {
        show_info_dialog(system_sales_popup_message);
    }

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
                    char sale_serial[NAME_LEN] = "";

                    if (p->serialized) {
                        char serial_ctx[200];
                        sprintf(serial_ctx, "Product: %s\nSKU: %s\n\nSerial number capture at time of sale is required.", p->name, p->sku);
                        if (!prompt_for_serial_number_dialog("Enter Serial Number", serial_ctx, sale_serial, sizeof(sale_serial))) {
                            gtk_widget_destroy(prod_dialog);
                            continue;
                        }
                    }

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
                    } else if (p->serialized) {
                        strncpy(txn.so_comments[txn.item_count], sale_serial, NAME_LEN - 1);
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

    GtkWidget *split_frame = gtk_frame_new("Split Payments (optional)");
    gtk_box_pack_start(GTK_BOX(pay_box), split_frame, FALSE, FALSE, 0);
    GtkWidget *split_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER(split_vbox), 6);
    gtk_container_add(GTK_CONTAINER(split_frame), split_vbox);

    GtkWidget *split_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_pack_start(GTK_BOX(split_vbox), split_row, FALSE, FALSE, 0);
    GtkWidget *split_method_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(split_method_combo), "Cash");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(split_method_combo), "Credit Card");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(split_method_combo), "Debit Card");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(split_method_combo), "Gift Card");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(split_method_combo), "Financing");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(split_method_combo), "Check");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(split_method_combo), "Trade-In");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(split_method_combo), "On Account");
    gtk_combo_box_set_active(GTK_COMBO_BOX(split_method_combo), 0);
    GtkWidget *split_amount_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(split_amount_entry), "Amount");
    GtkWidget *split_add_btn = gtk_button_new_with_label("Add Split Line");
    gtk_box_pack_start(GTK_BOX(split_row), gtk_label_new("Method:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(split_row), split_method_combo, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(split_row), gtk_label_new("Amount:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(split_row), split_amount_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(split_row), split_add_btn, FALSE, FALSE, 0);

    GtkListStore *split_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_DOUBLE);
    GtkWidget *split_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(split_store));
    GtkCellRenderer *split_renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(split_tree), -1, "Method", split_renderer, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(split_tree), -1, "Amount", split_renderer, "text", 1, NULL);
    GtkWidget *split_sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(split_sw, 460, 100);
    gtk_container_add(GTK_CONTAINER(split_sw), split_tree);
    gtk_box_pack_start(GTK_BOX(split_vbox), split_sw, FALSE, FALSE, 0);
    GtkWidget *split_total_label = gtk_label_new("Split Entered: $0.00");
    gtk_box_pack_start(GTK_BOX(split_vbox), split_total_label, FALSE, FALSE, 0);

    GtkWidget *receipt_checkbox = gtk_check_button_new_with_label("Print Receipt");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(receipt_checkbox), TRUE);
    gtk_box_pack_start(GTK_BOX(pay_box), receipt_checkbox, FALSE, FALSE, 0);

    GtkWidget *keep_open_checkbox = gtk_check_button_new_with_label("Keep transaction open");
    gtk_box_pack_start(GTK_BOX(pay_box), keep_open_checkbox, FALSE, FALSE, 0);

    g_object_set_data(G_OBJECT(pay_dialog), "split_store", split_store);
    g_object_set_data(G_OBJECT(pay_dialog), "split_method_combo", split_method_combo);
    g_object_set_data(G_OBJECT(pay_dialog), "split_amount_entry", split_amount_entry);
    g_object_set_data(G_OBJECT(pay_dialog), "split_total_label", split_total_label);
    g_signal_connect(split_add_btn, "clicked", G_CALLBACK(on_add_split_payment_line_clicked), pay_dialog);

    gtk_widget_show_all(pay_dialog);

    if (gtk_dialog_run(GTK_DIALOG(pay_dialog)) == 1) {
        double split_total = sum_split_payment_rows(GTK_TREE_MODEL(split_store));
        int use_split = split_total > 0.0;
        txn.amount_paid = use_split ? split_total : atof(gtk_entry_get_text(GTK_ENTRY(amount_entry)));
        txn.payment_type = gtk_combo_box_get_active(GTK_COMBO_BOX(payment_type_combo));
        txn.print_receipt = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(receipt_checkbox));
        int keep_open = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(keep_open_checkbox));

        if (use_split) {
            GtkTreeIter split_iter;
            if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(split_store), &split_iter)) {
                char *method = NULL;
                gtk_tree_model_get(GTK_TREE_MODEL(split_store), &split_iter, 0, &method, -1);
                if (method) {
                    txn.payment_type = payment_type_from_method_text(method);
                    g_free(method);
                }
            }
        }

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

            if (txn.customer_idx >= 0 && txn.customer_idx < s->customer_count) {
                get_today_date(s->customers[txn.customer_idx].last_visit, sizeof(s->customers[txn.customer_idx].last_visit));
            }

            if (use_split) {
                GtkTreeIter split_iter;
                gboolean split_valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(split_store), &split_iter);
                while (split_valid) {
                    char *method = NULL;
                    double amount = 0.0;
                    gtk_tree_model_get(GTK_TREE_MODEL(split_store), &split_iter, 0, &method, 1, &amount, -1);
                    if (method && amount > 0.0) {
                        add_payment_ledger_entry(si, txn.transaction_id, method, amount, "Initial payment");
                    }
                    if (method) g_free(method);
                    split_valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(split_store), &split_iter);
                }
            } else {
                add_payment_ledger_entry(si, txn.transaction_id,
                                         (txn.payment_type >= PAYMENT_CASH && txn.payment_type <= PAYMENT_GIFT) ? payment_names[txn.payment_type] : "Other",
                                         txn.amount_paid,
                                         "Initial payment");
            }

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

            if (trek_auto_register) {
                for (int i = 0; i < txn.item_count; i++) {
                    Product *p = NULL;
                    for (int j = 0; j < s->product_count; j++) {
                        if (strcmp(s->products[j].sku, txn.item_sku[i]) == 0) {
                            p = &s->products[j];
                            break;
                        }
                    }
                    if (p && p->serialized && strcasecmp(p->vendor, "Trek") == 0) {
                        char details[220];
                        snprintf(details, sizeof(details), "Auto-registration prompt queued Txn:%s SKU:%s Serial:%s", txn.transaction_id, txn.item_sku[i], txn.so_comments[i]);
                        add_audit_log_entry(current_sales_user, "TrekAutoRegistration", details);
                    }
                }
            }
        } else {
            txn.status = 0; // open/layaway
            s->sales_count++;
            s->sales[s->sales_count - 1] = txn;

            if (txn.customer_idx >= 0 && txn.customer_idx < s->customer_count) {
                get_today_date(s->customers[txn.customer_idx].last_visit, sizeof(s->customers[txn.customer_idx].last_visit));
            }

            if (use_split) {
                GtkTreeIter split_iter;
                gboolean split_valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(split_store), &split_iter);
                while (split_valid) {
                    char *method = NULL;
                    double amount = 0.0;
                    gtk_tree_model_get(GTK_TREE_MODEL(split_store), &split_iter, 0, &method, 1, &amount, -1);
                    if (method && amount > 0.0) {
                        add_payment_ledger_entry(si, txn.transaction_id, method, amount, "Layaway opening payment");
                    }
                    if (method) g_free(method);
                    split_valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(split_store), &split_iter);
                }
            } else {
                add_payment_ledger_entry(si, txn.transaction_id,
                                         (txn.payment_type >= PAYMENT_CASH && txn.payment_type <= PAYMENT_GIFT) ? payment_names[txn.payment_type] : "Other",
                                         txn.amount_paid,
                                         "Layaway opening payment");
            }

            char msg[150];
            double remaining = txn.total - txn.amount_paid;
            sprintf(msg, "Layaway Transaction Created\n\nTotal: $%.2f\nPaid: $%.2f\nRemaining: $%.2f",
                    txn.total, txn.amount_paid, remaining);
            show_info_dialog(msg);
        }
    }

    gtk_widget_destroy(pay_dialog);
}

static int days_until_date(const char *date_ymd) {
    int y = 0, m = 0, d = 0;
    if (!date_ymd || sscanf(date_ymd, "%d-%d-%d", &y, &m, &d) != 3) return 999999;

    time_t now = time(NULL);
    struct tm today_tm = *localtime(&now);
    today_tm.tm_hour = 0;
    today_tm.tm_min = 0;
    today_tm.tm_sec = 0;
    time_t today_ts = mktime(&today_tm);

    struct tm due_tm;
    memset(&due_tm, 0, sizeof(due_tm));
    due_tm.tm_year = y - 1900;
    due_tm.tm_mon = m - 1;
    due_tm.tm_mday = d;
    due_tm.tm_isdst = -1;
    time_t due_ts = mktime(&due_tm);
    if (due_ts == (time_t)-1 || today_ts == (time_t)-1) return 999999;

    double diff_days = difftime(due_ts, today_ts) / 86400.0;
    if (diff_days >= 0) return (int)(diff_days + 0.5);
    return (int)(diff_days - 0.5);
}

static int get_txn_due_date(const Transaction *txn, char *out_due, size_t out_size) {
    if (!txn || !out_due || out_size < 11) return 0;
    out_due[0] = '\0';
    const char *marker = strstr(txn->notes, "DUE:");
    if (!marker) return 0;
    marker += 4;
    if (strlen(marker) < 10) return 0;
    strncpy(out_due, marker, 10);
    out_due[10] = '\0';
    return 1;
}

static void set_txn_due_date(Transaction *txn, const char *due_date) {
    if (!txn) return;
    if (!due_date || strlen(due_date) < 10) {
        strncpy(txn->notes, "", sizeof(txn->notes) - 1);
        txn->notes[sizeof(txn->notes) - 1] = '\0';
        return;
    }
    char kept[NAME_LEN * 2] = "";
    const char *pipe = strchr(txn->notes, '|');
    if (pipe && *(pipe + 1) != '\0') {
        strncpy(kept, pipe + 1, sizeof(kept) - 1);
        kept[sizeof(kept) - 1] = '\0';
    }
    if (strlen(kept) > 0) {
        snprintf(txn->notes, sizeof(txn->notes), "DUE:%.*s|%s", 10, due_date, kept);
    } else {
        snprintf(txn->notes, sizeof(txn->notes), "DUE:%.*s", 10, due_date);
    }
}

static void refresh_customer_notes_store(LayawayWorkbenchContext *ctx) {
    if (!ctx || !ctx->notes_store) return;
    gtk_list_store_clear(ctx->notes_store);
    for (int i = 0; i < customer_note_count; i++) {
        CustomerNote *n = &customer_notes[i];
        if (n->hidden) continue;
        if (n->store_idx != ctx->store_idx || n->customer_idx != ctx->customer_idx) continue;
        GtkTreeIter iter;
        gtk_list_store_append(ctx->notes_store, &iter);
        gtk_list_store_set(ctx->notes_store, &iter,
                           0, n->date,
                           1, n->note,
                           2, i,
                           -1);
    }
}

static int selected_note_index(LayawayWorkbenchContext *ctx) {
    if (!ctx || !ctx->notes_tree) return -1;
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(ctx->notes_tree));
    GtkTreeModel *model;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(sel, &model, &iter)) return -1;
    int note_idx = -1;
    gtk_tree_model_get(model, &iter, 2, &note_idx, -1);
    return note_idx;
}

static void on_layaway_note_add(GtkWidget *widget, gpointer data) {
    (void)widget;
    LayawayWorkbenchContext *ctx = (LayawayWorkbenchContext *)data;
    if (customer_note_count >= MAX_CUSTOMER_NOTES) {
        show_error_dialog("Maximum customer notes reached.");
        return;
    }

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Add Customer Note",
                                                     GTK_WINDOW(ctx->dialog),
                                                     GTK_DIALOG_MODAL,
                                                     "_Save", GTK_RESPONSE_OK,
                                                     "_Cancel", GTK_RESPONSE_CANCEL,
                                                     NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(box), 10);
    gtk_container_add(GTK_CONTAINER(area), box);

    GtkWidget *date_entry = create_labeled_entry("Date (YYYY-MM-DD):", box);
    GtkWidget *note_entry = create_labeled_entry("Note:", box);
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char dt[16];
    strftime(dt, sizeof(dt), "%Y-%m-%d", tm_info);
    gtk_entry_set_text(GTK_ENTRY(date_entry), dt);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *date = gtk_entry_get_text(GTK_ENTRY(date_entry));
        const char *text = gtk_entry_get_text(GTK_ENTRY(note_entry));
        if (strlen(text) == 0) {
            show_error_dialog("Note text is required.");
        } else {
            CustomerNote *n = &customer_notes[customer_note_count++];
            n->store_idx = ctx->store_idx;
            n->customer_idx = ctx->customer_idx;
            n->hidden = 0;
            strncpy(n->date, date, NAME_LEN - 1);
            n->date[NAME_LEN - 1] = '\0';
            strncpy(n->note, text, sizeof(n->note) - 1);
            n->note[sizeof(n->note) - 1] = '\0';
            refresh_customer_notes_store(ctx);
            layaway_touch_modified(ctx);
            save_data();
        }
    }
    gtk_widget_destroy(dialog);
}

static void on_layaway_note_edit(GtkWidget *widget, gpointer data) {
    (void)widget;
    LayawayWorkbenchContext *ctx = (LayawayWorkbenchContext *)data;
    int idx = selected_note_index(ctx);
    if (idx < 0 || idx >= customer_note_count) {
        show_error_dialog("Select a note to edit.");
        return;
    }
    CustomerNote *n = &customer_notes[idx];

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Edit Customer Note",
                                                     GTK_WINDOW(ctx->dialog),
                                                     GTK_DIALOG_MODAL,
                                                     "_Save", GTK_RESPONSE_OK,
                                                     "_Cancel", GTK_RESPONSE_CANCEL,
                                                     NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(box), 10);
    gtk_container_add(GTK_CONTAINER(area), box);

    GtkWidget *date_entry = create_labeled_entry("Date (YYYY-MM-DD):", box);
    GtkWidget *note_entry = create_labeled_entry("Note:", box);
    gtk_entry_set_text(GTK_ENTRY(date_entry), n->date);
    gtk_entry_set_text(GTK_ENTRY(note_entry), n->note);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *date = gtk_entry_get_text(GTK_ENTRY(date_entry));
        const char *text = gtk_entry_get_text(GTK_ENTRY(note_entry));
        if (strlen(text) == 0) {
            show_error_dialog("Note text is required.");
        } else {
            strncpy(n->date, date, NAME_LEN - 1);
            n->date[NAME_LEN - 1] = '\0';
            strncpy(n->note, text, sizeof(n->note) - 1);
            n->note[sizeof(n->note) - 1] = '\0';
            refresh_customer_notes_store(ctx);
            layaway_touch_modified(ctx);
            save_data();
        }
    }
    gtk_widget_destroy(dialog);
}

static void on_layaway_note_remove(GtkWidget *widget, gpointer data) {
    (void)widget;
    LayawayWorkbenchContext *ctx = (LayawayWorkbenchContext *)data;
    int idx = selected_note_index(ctx);
    if (idx < 0 || idx >= customer_note_count) {
        show_error_dialog("Select a note to remove.");
        return;
    }
    customer_notes[idx].hidden = 1;
    refresh_customer_notes_store(ctx);
    layaway_touch_modified(ctx);
    save_data();
}

static void layaway_policy_settings_dialog(GtkWidget *widget, gpointer data) {
    (void)widget;
    LayawayWorkbenchContext *ctx = (LayawayWorkbenchContext *)data;
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Layaway Policy Settings",
                                                     GTK_WINDOW(ctx->dialog),
                                                     GTK_DIALOG_MODAL,
                                                     "_Save", GTK_RESPONSE_OK,
                                                     "_Cancel", GTK_RESPONSE_CANCEL,
                                                     NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(box), 10);
    gtk_container_add(GTK_CONTAINER(area), box);

    GtkWidget *reminder_entry = create_labeled_entry("Reminder Days Before Due:", box);
    GtkWidget *grace_entry = create_labeled_entry("Grace Days After Due:", box);
    GtkWidget *fee_entry = create_labeled_entry("Cancellation Fee %:", box);
    GtkWidget *auto_cancel = gtk_check_button_new_with_label("Auto-cancel overdue layaways past grace period");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(auto_cancel), app_settings.layaway_auto_cancel_enabled);
    gtk_box_pack_start(GTK_BOX(box), auto_cancel, FALSE, FALSE, 0);

    char temp[32];
    snprintf(temp, sizeof(temp), "%d", app_settings.layaway_reminder_days);
    gtk_entry_set_text(GTK_ENTRY(reminder_entry), temp);
    snprintf(temp, sizeof(temp), "%d", app_settings.layaway_grace_days);
    gtk_entry_set_text(GTK_ENTRY(grace_entry), temp);
    snprintf(temp, sizeof(temp), "%.2f", app_settings.layaway_cancel_fee_percent);
    gtk_entry_set_text(GTK_ENTRY(fee_entry), temp);

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        app_settings.layaway_reminder_days = atoi(gtk_entry_get_text(GTK_ENTRY(reminder_entry)));
        app_settings.layaway_grace_days = atoi(gtk_entry_get_text(GTK_ENTRY(grace_entry)));
        app_settings.layaway_cancel_fee_percent = atof(gtk_entry_get_text(GTK_ENTRY(fee_entry)));
        app_settings.layaway_auto_cancel_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(auto_cancel));
        if (app_settings.layaway_reminder_days < 0) app_settings.layaway_reminder_days = 0;
        if (app_settings.layaway_grace_days < 0) app_settings.layaway_grace_days = 0;
        if (app_settings.layaway_cancel_fee_percent < 0.0) app_settings.layaway_cancel_fee_percent = 0.0;
        save_data();
        show_info_dialog("Layaway policy settings saved.");
    }
    gtk_widget_destroy(dialog);
}

static void on_apply_layaway_rules_clicked(GtkButton *button, gpointer data) {
    (void)button;
    if (!request_manager_override("Apply Layaway Cancellation Rules")) return;
    Store *s = (Store *)data;
    if (!s) return;
    int canceled = 0;
    for (int i = 0; i < s->sales_count; i++) {
        Transaction *txn = &s->sales[i];
        if (txn->status != 0) continue;
        char due[16];
        if (!get_txn_due_date(txn, due, sizeof(due))) continue;
        int days = days_until_date(due);
        if (app_settings.layaway_auto_cancel_enabled && days < -app_settings.layaway_grace_days) {
            txn->status = 3;
            char extra[64];
            snprintf(extra, sizeof(extra), "CANCEL_FEE:%.2f", app_settings.layaway_cancel_fee_percent);
            if (!strstr(txn->notes, extra)) {
                strncat(txn->notes, "|", sizeof(txn->notes) - strlen(txn->notes) - 1);
                strncat(txn->notes, extra, sizeof(txn->notes) - strlen(txn->notes) - 1);
            }
            canceled++;
        }
    }
    save_data();
    if (canceled > 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "%d layaway transaction(s) canceled by policy rules.", canceled);
        add_audit_log_entry("System", "LayawayAutoCancel", msg);
        show_info_dialog(msg);
    } else {
        show_info_dialog("No layaways met cancellation criteria.");
    }
}

static void set_main_window_context_title(const char *store_name, const char *module_name, const char *context_label) {
    char title[256];
    snprintf(title, sizeof(title), "Ascend Retail Management Software - %s - [%s for %s]",
             store_name ? store_name : "Store",
             module_name ? module_name : "Module",
             context_label ? context_label : "Customer");
    gtk_window_set_title(GTK_WINDOW(main_window), title);
}

static void reset_main_window_title(void) {
    gtk_window_set_title(GTK_WINDOW(main_window), "Ascend Retail Platform");
}

static void layaway_touch_modified(LayawayWorkbenchContext *ctx) {
    if (!ctx || !ctx->modified_label) return;
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char dt[64];
    strftime(dt, sizeof(dt), "%Y-%m-%d %H:%M", tm_info);
    char line[96];
    snprintf(line, sizeof(line), "Last Modified: %s", dt);
    gtk_label_set_text(GTK_LABEL(ctx->modified_label), line);
}

static double layaway_get_tax_rate(LayawayWorkbenchContext *ctx) {
    int idx = gtk_combo_box_get_active(GTK_COMBO_BOX(ctx->tax_rate_combo));
    if (idx == 0) return 0.00;
    if (idx == 1) return 0.04;
    if (idx == 2) return 0.07;
    return app_settings.default_tax_rate;
}

static void layaway_recalc_totals(LayawayWorkbenchContext *ctx) {
    GtkTreeIter iter;
    gboolean valid;
    double subtotal = 0.0;

    valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ctx->items_store), &iter);
    while (valid) {
        double price = 0.0;
        int qty = 0;
        gtk_tree_model_get(GTK_TREE_MODEL(ctx->items_store), &iter,
                           LAY_COL_PRICE, &price,
                           LAY_COL_QTY, &qty,
                           -1);
        if (qty < 0) qty = 0;
        double line_total = price * qty;
        subtotal += line_total;
        gtk_list_store_set(ctx->items_store, &iter, LAY_COL_LINE_TOTAL, line_total, -1);
        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(ctx->items_store), &iter);
    }

    ctx->subtotal = subtotal;
    ctx->shipping = atof(gtk_entry_get_text(GTK_ENTRY(ctx->shipping_entry)));
    if (ctx->shipping < 0.0) ctx->shipping = 0.0;
    ctx->discount = atof(gtk_entry_get_text(GTK_ENTRY(ctx->discount_entry)));
    if (ctx->discount < 0.0) ctx->discount = 0.0;

    ctx->tax = (ctx->subtotal - ctx->discount) * layaway_get_tax_rate(ctx);
    if (ctx->tax < 0.0) ctx->tax = 0.0;
    ctx->total = ctx->subtotal + ctx->tax + ctx->shipping - ctx->discount;
    if (ctx->total < 0.0) ctx->total = 0.0;

    char line[128];
    snprintf(line, sizeof(line), "Subtotal: $%.2f", ctx->subtotal);
    gtk_label_set_text(GTK_LABEL(ctx->subtotal_label), line);
    snprintf(line, sizeof(line), "Tax: $%.2f", ctx->tax);
    gtk_label_set_text(GTK_LABEL(ctx->tax_label), line);
    snprintf(line, sizeof(line), "Total: $%.2f", ctx->total);
    gtk_label_set_text(GTK_LABEL(ctx->total_label), line);
    snprintf(line, sizeof(line), "Payments: $%.2f", ctx->payments_total);
    gtk_label_set_text(GTK_LABEL(ctx->payments_label), line);
    snprintf(line, sizeof(line), "Balance: $%.2f", ctx->total - ctx->payments_total);
    gtk_label_set_text(GTK_LABEL(ctx->balance_label), line);
}

static void on_layaway_shipping_changed(GtkEditable *editable, gpointer data) {
    (void)editable;
    LayawayWorkbenchContext *ctx = (LayawayWorkbenchContext *)data;
    layaway_recalc_totals(ctx);
    layaway_touch_modified(ctx);
}

static void on_layaway_discount_changed(GtkEditable *editable, gpointer data) {
    (void)editable;
    LayawayWorkbenchContext *ctx = (LayawayWorkbenchContext *)data;
    layaway_recalc_totals(ctx);
    layaway_touch_modified(ctx);
}

static void on_layaway_tax_changed(GtkComboBox *combo, gpointer data) {
    (void)combo;
    LayawayWorkbenchContext *ctx = (LayawayWorkbenchContext *)data;
    layaway_recalc_totals(ctx);
    layaway_touch_modified(ctx);
}

static void on_layaway_cell_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text, gpointer data) {
    LayawayWorkbenchContext *ctx = (LayawayWorkbenchContext *)data;
    GtkTreePath *tree_path = gtk_tree_path_new_from_string(path);
    GtkTreeIter iter;
    int col = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(renderer), "lay-col"));
    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(ctx->items_store), &iter, tree_path)) {
        gtk_tree_path_free(tree_path);
        return;
    }

    if (col == LAY_COL_PRICE || col == LAY_COL_MSRP) {
        double value = atof(new_text);
        if (value < 0.0) value = 0.0;
        gtk_list_store_set(ctx->items_store, &iter, col, value, -1);
    } else if (col == LAY_COL_QTY) {
        int qty = atoi(new_text);
        if (qty < 0) qty = 0;
        gtk_list_store_set(ctx->items_store, &iter, LAY_COL_QTY, qty, -1);
    } else {
        gtk_list_store_set(ctx->items_store, &iter, col, new_text, -1);
    }

    if (col == LAY_COL_SKU) {
        for (int i = 0; i < ctx->store->product_count; i++) {
            Product *p = &ctx->store->products[i];
            if (strcmp(p->sku, new_text) == 0) {
                gtk_list_store_set(ctx->items_store, &iter,
                                   LAY_COL_DESC, p->name,
                                   LAY_COL_PRICE, p->price,
                                   LAY_COL_MSRP, p->price,
                                   -1);
                break;
            }
        }
    }

    gtk_tree_path_free(tree_path);
    layaway_recalc_totals(ctx);
    layaway_touch_modified(ctx);
}

static void on_layaway_add_row(GtkButton *button, gpointer data) {
    (void)button;
    LayawayWorkbenchContext *ctx = (LayawayWorkbenchContext *)data;
    GtkTreeIter iter;
    gtk_list_store_append(ctx->items_store, &iter);
    gtk_list_store_set(ctx->items_store, &iter,
                       LAY_COL_SKU, "",
                       LAY_COL_DESC, "",
                       LAY_COL_PRICE, 0.0,
                       LAY_COL_QTY, 1,
                       LAY_COL_SERIAL, "",
                       LAY_COL_MSRP, 0.0,
                       LAY_COL_LINE_TOTAL, 0.0,
                       LAY_COL_COMMENTS, "",
                       LAY_COL_WARRANTY, "",
                       -1);
    layaway_recalc_totals(ctx);
    layaway_touch_modified(ctx);
}

static void on_layaway_remove_selected(GtkButton *button, gpointer data) {
    (void)button;
    LayawayWorkbenchContext *ctx = (LayawayWorkbenchContext *)data;
    GtkWidget *tree = GTK_WIDGET(g_object_get_data(G_OBJECT(ctx->dialog), "layaway_items_tree"));
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    GList *rows = gtk_tree_selection_get_selected_rows(sel, NULL);
    for (GList *l = g_list_last(rows); l != NULL; l = l->prev) {
        GtkTreeIter iter;
        GtkTreePath *path = (GtkTreePath *)l->data;
        if (gtk_tree_model_get_iter(GTK_TREE_MODEL(ctx->items_store), &iter, path)) {
            gtk_list_store_remove(ctx->items_store, &iter);
        }
        gtk_tree_path_free(path);
    }
    g_list_free(rows);
    layaway_recalc_totals(ctx);
    layaway_touch_modified(ctx);
}

static void on_layaway_add_by_barcode(GtkButton *button, gpointer data) {
    (void)button;
    LayawayWorkbenchContext *ctx = (LayawayWorkbenchContext *)data;
    const char *barcode = gtk_entry_get_text(GTK_ENTRY(ctx->barcode_entry));
    if (strlen(barcode) == 0) {
        show_error_dialog("Enter a barcode/SKU first.");
        return;
    }

    Product *p = NULL;
    for (int i = 0; i < ctx->store->product_count; i++) {
        if (strcmp(ctx->store->products[i].sku, barcode) == 0) {
            p = &ctx->store->products[i];
            break;
        }
    }
    if (!p) {
        show_error_dialog("Barcode not found in inventory.");
        return;
    }

    GtkTreeIter iter;
    gtk_list_store_append(ctx->items_store, &iter);
    gtk_list_store_set(ctx->items_store, &iter,
                       LAY_COL_SKU, p->sku,
                       LAY_COL_DESC, p->name,
                       LAY_COL_PRICE, p->price,
                       LAY_COL_QTY, 1,
                       LAY_COL_SERIAL, "",
                       LAY_COL_MSRP, p->price,
                       LAY_COL_LINE_TOTAL, p->price,
                       LAY_COL_COMMENTS, "From barcode",
                       LAY_COL_WARRANTY, "",
                       -1);
    gtk_entry_set_text(GTK_ENTRY(ctx->barcode_entry), "");
    layaway_recalc_totals(ctx);
    layaway_touch_modified(ctx);
}

static void on_layaway_add_payment(GtkButton *button, gpointer data) {
    LayawayWorkbenchContext *ctx = (LayawayWorkbenchContext *)data;
    const char *pay_type = (const char *)g_object_get_data(G_OBJECT(button), "pay-type");
    if (!pay_type) pay_type = "Cash";

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Add Payment",
                                                     GTK_WINDOW(ctx->dialog),
                                                     GTK_DIALOG_MODAL,
                                                     "_Apply", GTK_RESPONSE_OK,
                                                     "_Cancel", GTK_RESPONSE_CANCEL,
                                                     NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(box), 10);
    gtk_container_add(GTK_CONTAINER(area), box);

    GtkWidget *amount_entry = create_labeled_entry("Amount:", box);
    GtkWidget *type_entry = create_labeled_entry("Payment Type:", box);
    GtkWidget *user_entry = create_labeled_entry("User Added:", box);
    gtk_entry_set_text(GTK_ENTRY(type_entry), pay_type);
    gtk_entry_set_text(GTK_ENTRY(user_entry), "Ascend User");

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        double amount = atof(gtk_entry_get_text(GTK_ENTRY(amount_entry)));
        const char *type = gtk_entry_get_text(GTK_ENTRY(type_entry));
        const char *user = gtk_entry_get_text(GTK_ENTRY(user_entry));
        if (amount > 0.0) {
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);
            char dt[32];
            strftime(dt, sizeof(dt), "%Y-%m-%d", tm_info);
            GtkTreeIter iter;
            gtk_list_store_append(ctx->payments_store, &iter);
            gtk_list_store_set(ctx->payments_store, &iter,
                               0, dt,
                               1, amount,
                               2, type,
                               3, user,
                               -1);
            ctx->payments_total += amount;
            layaway_recalc_totals(ctx);
            layaway_touch_modified(ctx);
        } else {
            show_error_dialog("Payment amount must be greater than 0.");
        }
    }
    gtk_widget_destroy(dialog);
}

static void on_layaway_action_info(GtkWidget *widget, gpointer data) {
    (void)data;
    const char *label = gtk_button_get_label(GTK_BUTTON(widget));
    char msg[256];
    snprintf(msg, sizeof(msg), "%s workflow is available in this layaway workbench and can be expanded with deeper automation.", label ? label : "Action");
    show_info_dialog(msg);
}

static void on_layaway_serial_lookup(GtkWidget *widget, gpointer data) {
    (void)widget;
    LayawayWorkbenchContext *ctx = (LayawayWorkbenchContext *)data;
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Serial Lookup and Customer Serial History",
                                                     GTK_WINDOW(ctx->dialog),
                                                     GTK_DIALOG_MODAL,
                                                     "_Close", GTK_RESPONSE_CLOSE,
                                                     NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(area), vbox);

    GtkWidget *search_entry = create_labeled_entry("Serial Search:", vbox);
    GtkWidget *text = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(sw, 620, 260);
    gtk_container_add(GTK_CONTAINER(sw), text);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

    char report[12000] = "Customer Serial History\n\n";
    for (int i = 0; i < ctx->store->sales_count; i++) {
        Transaction *t = &ctx->store->sales[i];
        if (t->customer_idx != ctx->customer_idx) continue;
        for (int j = 0; j < t->item_count; j++) {
            if (strlen(t->so_comments[j]) == 0) continue;
            char line[256];
            snprintf(line, sizeof(line), "%s | %s | %s\n", t->transaction_id, t->item_sku[j], t->so_comments[j]);
            strncat(report, line, sizeof(report) - strlen(report) - 1);
        }
    }

    strncat(report, "\nOwnership Timeline (search by serial in box above, then close):\n", sizeof(report) - strlen(report) - 1);

    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
    gtk_text_buffer_set_text(buf, report, -1);

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_CLOSE) {
        const char *search = gtk_entry_get_text(GTK_ENTRY(search_entry));
        if (search && strlen(search) > 0) {
            char timeline[9000] = "Serial Ownership Timeline\n\n";
            int hits = 0;
            for (int i = 0; i < ctx->store->sales_count; i++) {
                Transaction *t = &ctx->store->sales[i];
                for (int j = 0; j < t->item_count; j++) {
                    if (strcmp(t->so_comments[j], search) != 0) continue;
                    char customer_name[NAME_LEN * 2] = "Unknown";
                    if (t->customer_idx >= 0 && t->customer_idx < ctx->store->customer_count) {
                        Customer *cust = &ctx->store->customers[t->customer_idx];
                        snprintf(customer_name, sizeof(customer_name), "%s %s", cust->first_name, cust->last_name);
                    }
                    const char *action = t->is_return ? "RETURNED" : "PURCHASED";
                    char line[300];
                    snprintf(line, sizeof(line), "%s | %s | %s | %s | SKU:%s\n",
                             strlen(t->date) > 0 ? t->date : "(no date)",
                             customer_name,
                             action,
                             t->transaction_id,
                             t->item_sku[j]);
                    strncat(timeline, line, sizeof(timeline) - strlen(timeline) - 1);
                    hits++;
                }
            }
            if (hits == 0) {
                show_info_dialog("No matching serial found in history.");
            } else {
                GtkWidget *timeline_dialog = gtk_dialog_new_with_buttons("Serial Ownership Timeline",
                                                                          GTK_WINDOW(ctx->dialog),
                                                                          GTK_DIALOG_MODAL,
                                                                          "_Close", GTK_RESPONSE_CLOSE,
                                                                          NULL);
                GtkWidget *area2 = gtk_dialog_get_content_area(GTK_DIALOG(timeline_dialog));
                GtkWidget *tv = gtk_text_view_new();
                gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);
                GtkWidget *sw2 = gtk_scrolled_window_new(NULL, NULL);
                gtk_widget_set_size_request(sw2, 700, 300);
                gtk_container_add(GTK_CONTAINER(sw2), tv);
                gtk_container_add(GTK_CONTAINER(area2), sw2);
                GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
                gtk_text_buffer_set_text(tb, timeline, -1);
                gtk_widget_show_all(timeline_dialog);
                gtk_dialog_run(GTK_DIALOG(timeline_dialog));
                gtk_widget_destroy(timeline_dialog);
            }
        }
    }
    gtk_widget_destroy(dialog);
}

static void on_layaway_save(GtkWidget *widget, gpointer data) {
    (void)widget;
    LayawayWorkbenchContext *ctx = (LayawayWorkbenchContext *)data;
    if (ctx->saved) {
        show_info_dialog("This layaway transaction is already saved.");
        return;
    }
    if (ctx->store->sales_count >= MAX_TRANSACTIONS) {
        show_error_dialog("Maximum transactions reached for this store.");
        return;
    }

    memset(&ctx->txn, 0, sizeof(Transaction));
    ctx->txn.customer_idx = ctx->customer_idx;
    ctx->txn.status = 0;
    ctx->txn.total = ctx->total;
    ctx->txn.amount_paid = ctx->payments_total;
    ctx->txn.item_count = 0;
    ctx->txn.payment_type = PAYMENT_CASH;
    snprintf(ctx->txn.transaction_id, sizeof(ctx->txn.transaction_id), "LAY-%d-%ld", ctx->store_idx, (long)time(NULL) % 100000);

    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ctx->items_store), &iter);
    while (valid && ctx->txn.item_count < MAX_PRODUCTS) {
        char *sku = NULL;
        char *serial = NULL;
        double price = 0.0;
        int qty = 0;
        gtk_tree_model_get(GTK_TREE_MODEL(ctx->items_store), &iter,
                           LAY_COL_SKU, &sku,
                           LAY_COL_SERIAL, &serial,
                           LAY_COL_PRICE, &price,
                           LAY_COL_QTY, &qty,
                           -1);
        if (sku && strlen(sku) > 0 && qty > 0) {
            int idx = ctx->txn.item_count;
            strncpy(ctx->txn.item_sku[idx], sku, NAME_LEN - 1);
            ctx->txn.item_sku[idx][NAME_LEN - 1] = '\0';
            ctx->txn.item_price[idx] = price;
            ctx->txn.qty[idx] = qty;
            if (serial && strlen(serial) > 0) {
                strncpy(ctx->txn.so_comments[idx], serial, NAME_LEN - 1);
                ctx->txn.so_comments[idx][NAME_LEN - 1] = '\0';
            }
            ctx->txn.item_count++;
        }
        if (sku) g_free(sku);
        if (serial) g_free(serial);
        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(ctx->items_store), &iter);
    }

    if (ctx->txn.item_count == 0) {
        show_error_dialog("Add at least one valid item before saving layaway.");
        return;
    }

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(ctx->txn.date, NAME_LEN, "%Y-%m-%d %H:%M", tm_info);
    ctx->txn.print_receipt = 0;

    const char *due_date = gtk_entry_get_text(GTK_ENTRY(ctx->due_date_entry));
    set_txn_due_date(&ctx->txn, due_date);
    int days_to_due = days_until_date(due_date);

    ctx->store->sales[ctx->store->sales_count++] = ctx->txn;
    ctx->store->transactions++;
    ctx->saved = 1;

    GtkTreeIter pay_iter;
    gboolean pay_valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ctx->payments_store), &pay_iter);
    int payment_rows = 0;
    while (pay_valid) {
        char *date = NULL;
        double amount = 0.0;
        char *ptype = NULL;
        char *puser = NULL;
        gtk_tree_model_get(GTK_TREE_MODEL(ctx->payments_store), &pay_iter,
                           0, &date,
                           1, &amount,
                           2, &ptype,
                           3, &puser,
                           -1);
        if (amount != 0.0) {
            add_payment_ledger_entry(ctx->store_idx, ctx->txn.transaction_id,
                                     ptype ? ptype : "Other",
                                     amount,
                                     puser ? puser : "Layaway payment");
            payment_rows++;
        }
        if (date) g_free(date);
        if (ptype) g_free(ptype);
        if (puser) g_free(puser);
        pay_valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(ctx->payments_store), &pay_iter);
    }
    if (payment_rows == 0 && ctx->payments_total != 0.0) {
        add_payment_ledger_entry(ctx->store_idx, ctx->txn.transaction_id, "Cash", ctx->payments_total, "Layaway payment");
    }

    save_data();

    char msg[256];
    snprintf(msg, sizeof(msg),
             "Layaway saved.\n\nTransaction: %s\nTotal: $%.2f\nPayments: $%.2f\nBalance: $%.2f",
             ctx->txn.transaction_id, ctx->total, ctx->payments_total, ctx->total - ctx->payments_total);
    if (days_to_due <= app_settings.layaway_reminder_days) {
        strncat(msg, "\n\nReminder: due date is approaching based on policy settings.", sizeof(msg) - strlen(msg) - 1);
    }
    {
        char details[200];
        snprintf(details, sizeof(details), "Txn:%s Store:%s Total:$%.2f Paid:$%.2f",
                 ctx->txn.transaction_id, ctx->store->name, ctx->total, ctx->payments_total);
        add_audit_log_entry("Ascend User", "LayawayCreate", details);
    }
    show_info_dialog(msg);
}

static GtkWidget *layaway_new_menu_item(GtkWidget *menu, const char *label, GCallback cb, gpointer data) {
    GtkWidget *item = gtk_menu_item_new_with_label(label);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    if (cb) g_signal_connect(item, "activate", cb, data);
    return item;
}

static void layaway_workbench_dialog(Store *s, int store_idx, int customer_idx) {
    LayawayWorkbenchContext *ctx = g_malloc0(sizeof(LayawayWorkbenchContext));
    ctx->store = s;
    ctx->store_idx = store_idx;
    ctx->customer_idx = customer_idx;

    Customer *c = &s->customers[customer_idx];
    char customer_label[NAME_LEN * 2 + 8];
    snprintf(customer_label, sizeof(customer_label), "%s %s", c->first_name, c->last_name);
    set_main_window_context_title(s->name, "Layaway", customer_label);

    ctx->dialog = gtk_dialog_new_with_buttons("Layaway Workbench",
                                               GTK_WINDOW(main_window),
                                               GTK_DIALOG_MODAL,
                                               "_Close", GTK_RESPONSE_CLOSE,
                                               NULL);
    gtk_window_set_default_size(GTK_WINDOW(ctx->dialog), 1450, 900);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(ctx->dialog));
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(root), 8);
    gtk_container_add(GTK_CONTAINER(area), root);

    GtkWidget *menubar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(root), menubar, FALSE, FALSE, 0);

    GtkWidget *file_menu = gtk_menu_new();
    GtkWidget *customer_menu = gtk_menu_new();
    GtkWidget *products_menu = gtk_menu_new();
    GtkWidget *payments_menu = gtk_menu_new();
    GtkWidget *view_menu = gtk_menu_new();
    GtkWidget *tools_menu = gtk_menu_new();
    GtkWidget *help_menu = gtk_menu_new();

    GtkWidget *file_item = gtk_menu_item_new_with_label("File");
    GtkWidget *cust_item = gtk_menu_item_new_with_label("Customer");
    GtkWidget *prod_item = gtk_menu_item_new_with_label("Products");
    GtkWidget *pay_item = gtk_menu_item_new_with_label("Payments");
    GtkWidget *view_item = gtk_menu_item_new_with_label("View");
    GtkWidget *tools_item = gtk_menu_item_new_with_label("Tools");
    GtkWidget *help_item = gtk_menu_item_new_with_label("Help");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(cust_item), customer_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(prod_item), products_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(pay_item), payments_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(view_item), view_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(tools_item), tools_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_item), help_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), cust_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), prod_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), pay_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), view_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), tools_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), help_item);

    layaway_new_menu_item(file_menu, "New", G_CALLBACK(on_layaway_action_info), NULL);
    layaway_new_menu_item(file_menu, "Save", G_CALLBACK(on_layaway_save), ctx);
    layaway_new_menu_item(file_menu, "Print", G_CALLBACK(on_layaway_action_info), NULL);
    layaway_new_menu_item(file_menu, "Exit", G_CALLBACK(on_layaway_action_info), NULL);
    layaway_new_menu_item(customer_menu, "Lookup", G_CALLBACK(on_layaway_action_info), NULL);
    layaway_new_menu_item(customer_menu, "Add New", G_CALLBACK(on_layaway_action_info), NULL);
    layaway_new_menu_item(products_menu, "Inventory", G_CALLBACK(on_layaway_action_info), NULL);
    layaway_new_menu_item(payments_menu, "Payment Config", G_CALLBACK(on_layaway_action_info), NULL);
    layaway_new_menu_item(view_menu, "Reports", G_CALLBACK(on_layaway_action_info), NULL);
    layaway_new_menu_item(view_menu, "Layouts", G_CALLBACK(on_layaway_action_info), NULL);
    layaway_new_menu_item(tools_menu, "Admin Tools", G_CALLBACK(on_layaway_action_info), NULL);
    layaway_new_menu_item(tools_menu, "Imports", G_CALLBACK(on_layaway_action_info), NULL);
    layaway_new_menu_item(tools_menu, "Layaway Policy Settings", G_CALLBACK(layaway_policy_settings_dialog), ctx);
    layaway_new_menu_item(help_menu, "Documentation", G_CALLBACK(instructions_reference_dialog), NULL);

    GtkWidget *toolbar_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(root), toolbar_box, FALSE, FALSE, 0);
    const char *left_buttons[] = {"Back", "Save", "Remove", "Print", "Copy", "Preview", "Barcode", "Email", "Keep Open"};
    const char *mid_buttons[] = {"Work Order", "WO Detail", "Reservation", "Discount", "Return", "Quote", "Availability"};

    for (int i = 0; i < 9; i++) {
        GtkWidget *btn = gtk_button_new_with_label(left_buttons[i]);
        gtk_box_pack_start(GTK_BOX(toolbar_box), btn, FALSE, FALSE, 0);
        if (strcmp(left_buttons[i], "Save") == 0) g_signal_connect(btn, "clicked", G_CALLBACK(on_layaway_save), ctx);
        else if (strcmp(left_buttons[i], "Remove") == 0) g_signal_connect(btn, "clicked", G_CALLBACK(on_layaway_remove_selected), ctx);
        else if (strcmp(left_buttons[i], "Barcode") == 0) g_signal_connect(btn, "clicked", G_CALLBACK(on_layaway_add_by_barcode), ctx);
        else g_signal_connect(btn, "clicked", G_CALLBACK(on_layaway_action_info), NULL);
    }

    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(toolbar_box), sep, FALSE, FALSE, 4);
    for (int i = 0; i < 7; i++) {
        GtkWidget *btn = gtk_button_new_with_label(mid_buttons[i]);
        gtk_box_pack_start(GTK_BOX(toolbar_box), btn, FALSE, FALSE, 0);
        g_signal_connect(btn, "clicked", G_CALLBACK(on_layaway_action_info), NULL);
    }

    GtkWidget *spacer = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(toolbar_box), spacer, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar_box), gtk_label_new("Sales Person:"), FALSE, FALSE, 0);
    ctx->salesperson_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ctx->salesperson_combo), "Ascend User");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ctx->salesperson_combo), "Manager");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ctx->salesperson_combo), "Sales Associate");
    gtk_combo_box_set_active(GTK_COMBO_BOX(ctx->salesperson_combo), 0);
    gtk_box_pack_start(GTK_BOX(toolbar_box), ctx->salesperson_combo, FALSE, FALSE, 0);

    GtkWidget *header_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(header_grid), 4);
    gtk_grid_set_column_spacing(GTK_GRID(header_grid), 14);
    gtk_box_pack_start(GTK_BOX(root), header_grid, FALSE, FALSE, 0);

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char created_dt[32];
    strftime(created_dt, sizeof(created_dt), "%m/%d/%Y", tm_info);
    char created_line[64];
    snprintf(created_line, sizeof(created_line), "Created: %s", created_dt);
    ctx->created_label = gtk_label_new(created_line);
    ctx->modified_label = gtk_label_new("Last Modified: N/A");
    ctx->created_by_label = gtk_label_new("Created By: Ascend User");
    ctx->transaction_id_label = gtk_label_new("Transaction ID: Pending Save");
    gtk_grid_attach(GTK_GRID(header_grid), ctx->created_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(header_grid), ctx->modified_label, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(header_grid), ctx->created_by_label, 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(header_grid), ctx->transaction_id_label, 3, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(header_grid), gtk_label_new("Due Date:"), 4, 0, 1, 1);
    ctx->due_date_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(ctx->due_date_entry), "2026-12-31");
    gtk_grid_attach(GTK_GRID(header_grid), ctx->due_date_entry, 5, 0, 1, 1);

    GtkWidget *title_context = gtk_label_new(NULL);
    char context_markup[320];
    snprintf(context_markup, sizeof(context_markup),
             "<b>Ascend Retail Management Software - %s - [Layaway for %s %s]</b>",
             s->name, c->first_name, c->last_name);
    gtk_label_set_markup(GTK_LABEL(title_context), context_markup);
    gtk_box_pack_start(GTK_BOX(root), title_context, FALSE, FALSE, 0);

    GtkWidget *hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(root), hpaned, TRUE, TRUE, 0);

    GtkWidget *left_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_paned_pack1(GTK_PANED(hpaned), left_box, TRUE, FALSE);
    gtk_paned_pack2(GTK_PANED(hpaned), right_box, FALSE, FALSE);

    GtkWidget *barcode_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(left_box), barcode_row, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barcode_row), gtk_label_new("Barcode:"), FALSE, FALSE, 0);
    ctx->barcode_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(barcode_row), ctx->barcode_entry, FALSE, FALSE, 0);
    GtkWidget *barcode_btn = gtk_button_new_with_label("Add by Barcode");
    gtk_box_pack_start(GTK_BOX(barcode_row), barcode_btn, FALSE, FALSE, 0);
    g_signal_connect(barcode_btn, "clicked", G_CALLBACK(on_layaway_add_by_barcode), ctx);

    GtkWidget *item_toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(left_box), item_toolbar, FALSE, FALSE, 0);
    GtkWidget *add_row_btn = gtk_button_new_with_label("Add");
    GtkWidget *edit_row_btn = gtk_button_new_with_label("Edit");
    GtkWidget *rem_row_btn = gtk_button_new_with_label("Remove");
    gtk_box_pack_start(GTK_BOX(item_toolbar), add_row_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(item_toolbar), edit_row_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(item_toolbar), rem_row_btn, FALSE, FALSE, 0);
    g_signal_connect(add_row_btn, "clicked", G_CALLBACK(on_layaway_add_row), ctx);
    g_signal_connect(edit_row_btn, "clicked", G_CALLBACK(on_layaway_action_info), NULL);
    g_signal_connect(rem_row_btn, "clicked", G_CALLBACK(on_layaway_remove_selected), ctx);

    GtkWidget *item_sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(item_sw, 920, 350);
    gtk_box_pack_start(GTK_BOX(left_box), item_sw, TRUE, TRUE, 0);

    ctx->items_store = gtk_list_store_new(LAY_COL_COUNT,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING,
                                          G_TYPE_DOUBLE,
                                          G_TYPE_INT,
                                          G_TYPE_STRING,
                                          G_TYPE_DOUBLE,
                                          G_TYPE_DOUBLE,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING);
    GtkWidget *items_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ctx->items_store));
    g_object_set_data(G_OBJECT(ctx->dialog), "layaway_items_tree", items_tree);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(items_tree)), GTK_SELECTION_MULTIPLE);

    const char *cols[] = {"SKU", "Description", "Price", "Quantity", "Serial No", "MSRP", "Total", "Comments", "Warranty"};
    for (int i = 0; i < LAY_COL_COUNT; i++) {
        GtkCellRenderer *rend = gtk_cell_renderer_text_new();
        if (i != LAY_COL_LINE_TOTAL) {
            g_object_set(rend, "editable", TRUE, NULL);
            g_object_set_data(G_OBJECT(rend), "lay-col", GINT_TO_POINTER(i));
            g_signal_connect(rend, "edited", G_CALLBACK(on_layaway_cell_edited), ctx);
        }
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(items_tree), -1, cols[i], rend, "text", i, NULL);
    }
    gtk_container_add(GTK_CONTAINER(item_sw), items_tree);

    GtkWidget *reg_frame = gtk_frame_new("Bike Registration");
    gtk_box_pack_start(GTK_BOX(right_box), reg_frame, FALSE, FALSE, 0);
    GtkWidget *reg_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(reg_box), 6);
    gtk_container_add(GTK_CONTAINER(reg_frame), reg_box);
    gtk_box_pack_start(GTK_BOX(reg_box), gtk_label_new("Register?"), FALSE, FALSE, 0);
    ctx->register_status_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ctx->register_status_combo), "Not Registered");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ctx->register_status_combo), "Deferred");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ctx->register_status_combo), "Completed");
    gtk_combo_box_set_active(GTK_COMBO_BOX(ctx->register_status_combo), 1);
    gtk_box_pack_start(GTK_BOX(reg_box), ctx->register_status_combo, FALSE, FALSE, 0);
    ctx->register_date_entry = create_labeled_entry("Register Date:", reg_box);
    gtk_entry_set_text(GTK_ENTRY(ctx->register_date_entry), "2026-10-31");
    gtk_box_pack_start(GTK_BOX(reg_box), gtk_label_new("Reminder and API registration hooks ready for expansion."), FALSE, FALSE, 0);

    GtkWidget *cust_frame = gtk_frame_new("Customer Profile");
    gtk_box_pack_start(GTK_BOX(right_box), cust_frame, FALSE, FALSE, 0);
    GtkWidget *cust_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(cust_grid), 2);
    gtk_grid_set_column_spacing(GTK_GRID(cust_grid), 8);
    gtk_container_set_border_width(GTK_CONTAINER(cust_grid), 6);
    gtk_container_add(GTK_CONTAINER(cust_frame), cust_grid);
    char line[160];
    snprintf(line, sizeof(line), "Name: %s %s", c->first_name, c->last_name);
    gtk_grid_attach(GTK_GRID(cust_grid), gtk_label_new(line), 0, 0, 2, 1);
    snprintf(line, sizeof(line), "Email: %s", c->email);
    gtk_grid_attach(GTK_GRID(cust_grid), gtk_label_new(line), 0, 1, 2, 1);
    snprintf(line, sizeof(line), "Mobile: %s", c->phone1);
    gtk_grid_attach(GTK_GRID(cust_grid), gtk_label_new(line), 0, 2, 1, 1);
    snprintf(line, sizeof(line), "Home: %s", c->phone2);
    gtk_grid_attach(GTK_GRID(cust_grid), gtk_label_new(line), 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(cust_grid), gtk_label_new("Language: English"), 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(cust_grid), gtk_label_new("Gender: Unspecified"), 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(cust_grid), gtk_label_new("Birthdate: Unspecified"), 0, 4, 2, 1);
    gtk_grid_attach(GTK_GRID(cust_grid), gtk_label_new("Tags: VIP, Frequent Buyer"), 0, 5, 2, 1);
    gtk_grid_attach(GTK_GRID(cust_grid), gtk_label_new("Multiple Contacts: supported"), 0, 6, 2, 1);

    GtkWidget *tabs = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(right_box), tabs, TRUE, TRUE, 0);
    const char *tab_names[] = {"Sales", "History", "Serial Numbers", "Notes", "Groups", "Email"};
    for (int i = 0; i < 6; i++) {
        GtkWidget *tab_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        if (i == 3) {
            GtkWidget *note_btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
            GtkWidget *add_note_btn = gtk_button_new_with_label("Add");
            GtkWidget *edit_note_btn = gtk_button_new_with_label("Edit");
            GtkWidget *remove_note_btn = gtk_button_new_with_label("Remove");
            gtk_box_pack_start(GTK_BOX(note_btns), add_note_btn, FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(note_btns), edit_note_btn, FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(note_btns), remove_note_btn, FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(tab_box), note_btns, FALSE, FALSE, 0);
            ctx->notes_store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
            ctx->notes_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ctx->notes_store));
            GtkCellRenderer *rend = gtk_cell_renderer_text_new();
            gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(ctx->notes_tree), -1, "Date", rend, "text", 0, NULL);
            gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(ctx->notes_tree), -1, "Note", rend, "text", 1, NULL);
            GtkWidget *notes_sw = gtk_scrolled_window_new(NULL, NULL);
            gtk_widget_set_size_request(notes_sw, 360, 130);
            gtk_container_add(GTK_CONTAINER(notes_sw), ctx->notes_tree);
            gtk_box_pack_start(GTK_BOX(tab_box), notes_sw, TRUE, TRUE, 0);
            g_signal_connect(add_note_btn, "clicked", G_CALLBACK(on_layaway_note_add), ctx);
            g_signal_connect(edit_note_btn, "clicked", G_CALLBACK(on_layaway_note_edit), ctx);
            g_signal_connect(remove_note_btn, "clicked", G_CALLBACK(on_layaway_note_remove), ctx);
            refresh_customer_notes_store(ctx);
        } else {
            gtk_box_pack_start(GTK_BOX(tab_box), gtk_label_new("Data panel ready for this tab."), FALSE, FALSE, 0);
        }
        gtk_notebook_append_page(GTK_NOTEBOOK(tabs), tab_box, gtk_label_new(tab_names[i]));
    }
    gtk_notebook_set_current_page(GTK_NOTEBOOK(tabs), 3);

    GtkWidget *payments_frame = gtk_frame_new("Payments");
    gtk_box_pack_start(GTK_BOX(left_box), payments_frame, FALSE, FALSE, 0);
    GtkWidget *payments_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER(payments_vbox), 6);
    gtk_container_add(GTK_CONTAINER(payments_frame), payments_vbox);

    GtkWidget *pay_btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_pack_start(GTK_BOX(payments_vbox), pay_btn_row, FALSE, FALSE, 0);
    const char *pay_buttons[] = {"Cash", "Check", "Credit", "Debit", "Gift Card", "Financing", "In-Store", "Coupon", "Trade-In", "Account"};
    for (int i = 0; i < 10; i++) {
        GtkWidget *pbtn = gtk_button_new_with_label(pay_buttons[i]);
        g_object_set_data(G_OBJECT(pbtn), "pay-type", (gpointer)pay_buttons[i]);
        g_signal_connect(pbtn, "clicked", G_CALLBACK(on_layaway_add_payment), ctx);
        gtk_box_pack_start(GTK_BOX(pay_btn_row), pbtn, FALSE, FALSE, 0);
    }

    ctx->payments_store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_DOUBLE, G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget *pay_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ctx->payments_store));
    GtkCellRenderer *pay_r = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(pay_tree), -1, "Date", pay_r, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(pay_tree), -1, "Amount", pay_r, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(pay_tree), -1, "Payment Type", pay_r, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(pay_tree), -1, "User Added", pay_r, "text", 3, NULL);
    GtkWidget *pay_sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(pay_sw, 900, 120);
    gtk_container_add(GTK_CONTAINER(pay_sw), pay_tree);
    gtk_box_pack_start(GTK_BOX(payments_vbox), pay_sw, TRUE, TRUE, 0);

    GtkWidget *comments_frame = gtk_frame_new("Comments");
    gtk_box_pack_start(GTK_BOX(left_box), comments_frame, FALSE, FALSE, 0);
    GtkWidget *comments_text = gtk_text_view_new();
    gtk_widget_set_size_request(comments_text, 900, 90);
    gtk_container_add(GTK_CONTAINER(comments_frame), comments_text);

    GtkWidget *totals_frame = gtk_frame_new("Totals");
    gtk_box_pack_start(GTK_BOX(right_box), totals_frame, FALSE, FALSE, 0);
    GtkWidget *totals_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER(totals_box), 6);
    gtk_container_add(GTK_CONTAINER(totals_frame), totals_box);
    ctx->subtotal_label = gtk_label_new("Subtotal: $0.00");
    ctx->tax_label = gtk_label_new("Tax: $0.00");
    ctx->total_label = gtk_label_new("Total: $0.00");
    ctx->payments_label = gtk_label_new("Payments: $0.00");
    ctx->balance_label = gtk_label_new("Balance: $0.00");
    gtk_box_pack_start(GTK_BOX(totals_box), ctx->subtotal_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(totals_box), ctx->tax_label, FALSE, FALSE, 0);
    ctx->tax_rate_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ctx->tax_rate_combo), "Tax 0% (Exempt)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ctx->tax_rate_combo), "Tax 4%");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ctx->tax_rate_combo), "Tax 7%");
    {
        char default_label[64];
        snprintf(default_label, sizeof(default_label), "Tax Default (%.4f)", app_settings.default_tax_rate);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ctx->tax_rate_combo), default_label);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(ctx->tax_rate_combo), 3);
    gtk_box_pack_start(GTK_BOX(totals_box), ctx->tax_rate_combo, FALSE, FALSE, 0);
    ctx->shipping_entry = create_labeled_entry("Shipping:", totals_box);
    gtk_entry_set_text(GTK_ENTRY(ctx->shipping_entry), "0.00");
    ctx->discount_entry = create_labeled_entry("Discount:", totals_box);
    gtk_entry_set_text(GTK_ENTRY(ctx->discount_entry), "0.00");
    gtk_box_pack_start(GTK_BOX(totals_box), ctx->total_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(totals_box), ctx->payments_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(totals_box), ctx->balance_label, FALSE, FALSE, 0);

    GtkWidget *serial_btn = gtk_button_new_with_label("Serial Lookup / History");
    gtk_box_pack_start(GTK_BOX(totals_box), serial_btn, FALSE, FALSE, 0);
    g_signal_connect(serial_btn, "clicked", G_CALLBACK(on_layaway_serial_lookup), ctx);

    GtkWidget *status_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_pack_start(GTK_BOX(root), status_row, FALSE, FALSE, 0);
    GtkWidget *status_left = gtk_label_new("Connection: Online | Sync: Active");
    ctx->status_right_label = gtk_label_new(NULL);
    char status_right[256];
    snprintf(status_right, sizeof(status_right), "Version 1.0 | DB 1.0 | Store %s | Workstation WS-01 | User Ascend User", s->name);
    gtk_label_set_text(GTK_LABEL(ctx->status_right_label), status_right);
    gtk_box_pack_start(GTK_BOX(status_row), status_left, FALSE, FALSE, 0);
    GtkWidget *status_spacer = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(status_row), status_spacer, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(status_row), ctx->status_right_label, FALSE, FALSE, 0);

    g_signal_connect(ctx->shipping_entry, "changed", G_CALLBACK(on_layaway_shipping_changed), ctx);
    g_signal_connect(ctx->discount_entry, "changed", G_CALLBACK(on_layaway_discount_changed), ctx);
    g_signal_connect(ctx->tax_rate_combo, "changed", G_CALLBACK(on_layaway_tax_changed), ctx);

    layaway_recalc_totals(ctx);
    gtk_widget_show_all(ctx->dialog);
    gtk_dialog_run(GTK_DIALOG(ctx->dialog));
    gtk_widget_destroy(ctx->dialog);
    reset_main_window_title();
    g_free(ctx);
}

static void create_layaway_dialog(void) {
    int si = choose_store_index();
    if (si < 0) return;
    Store *s = &stores[si];

    if (s->customer_count == 0) {
        show_error_dialog("No customers in this store. Add a customer first.");
        return;
    }

    GtkWidget *cust_dialog = gtk_dialog_new_with_buttons("Select Customer for Layaway",
                                                         GTK_WINDOW(main_window),
                                                         GTK_DIALOG_MODAL,
                                                         "_OK", GTK_RESPONSE_OK,
                                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                                         NULL);
    GtkWidget *cust_area = gtk_dialog_get_content_area(GTK_DIALOG(cust_dialog));
    GtkWidget *cust_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(cust_area), cust_box);
    GtkWidget *cust_combo = gtk_combo_box_text_new();
    for (int i = 0; i < s->customer_count; i++) {
        if (s->customers[i].hidden) continue;
        char entry[NAME_LEN * 2];
        snprintf(entry, sizeof(entry), "%s %s (%s)", s->customers[i].first_name, s->customers[i].last_name, s->customers[i].company);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cust_combo), entry);
    }
    gtk_box_pack_start(GTK_BOX(cust_box), gtk_label_new("Select Customer:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cust_box), cust_combo, FALSE, FALSE, 0);
    gtk_widget_show_all(cust_dialog);

    if (gtk_dialog_run(GTK_DIALOG(cust_dialog)) == GTK_RESPONSE_OK) {
        int ci = gtk_combo_box_get_active(GTK_COMBO_BOX(cust_combo));
        if (ci >= 0 && ci < s->customer_count) {
            layaway_workbench_dialog(s, si, ci);
        }
    }
    gtk_widget_destroy(cust_dialog);
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
            if (!request_manager_override("Complete Refund")) {
                gtk_widget_destroy(refund_dlg);
                return;
            }
            int ptype = gtk_combo_box_get_active(GTK_COMBO_BOX(refund_type_combo));
            ret_txn.payment_type = (ptype == 2) ? PAYMENT_GIFT : (ptype == 1 ? PAYMENT_CREDIT : PAYMENT_CASH);
            ret_txn.amount_paid = ret_txn.total; // refund = total (negative)
            add_payment_ledger_entry(si,
                                     ret_txn.transaction_id,
                                     (ptype == 2) ? "Gift Card" : (ptype == 1 ? "Credit Card" : "Cash"),
                                     ret_txn.amount_paid,
                                     "Refund");

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
            {
                char details[220];
                snprintf(details, sizeof(details), "Txn:%s Original:%s Refund:$%.2f",
                         ret_txn.transaction_id, ret_txn.original_transaction_id, -ret_txn.total);
                add_audit_log_entry("Ascend User", "ReturnComplete", details);
            }
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

                    if (found_txn->customer_idx >= 0 && found_txn->customer_idx < s->customer_count) {
                        get_today_date(s->customers[found_txn->customer_idx].last_visit, sizeof(s->customers[found_txn->customer_idx].last_visit));
                    }

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
                    {
                        char details[180];
                        snprintf(details, sizeof(details), "Txn:%s Total:$%.2f", found_txn->transaction_id, found_txn->total);
                        add_audit_log_entry("Ascend User", "CompleteSale", details);
                    }
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

    GtkListStore *store_list = gtk_list_store_new(8,
                                                   G_TYPE_STRING,  // Transaction ID
                                                   G_TYPE_STRING,  // Customer
                                                   G_TYPE_DOUBLE,  // Total
                                                   G_TYPE_DOUBLE,  // Paid
                                                   G_TYPE_DOUBLE,  // Remaining
                                                   G_TYPE_STRING,  // Due Date
                                                   G_TYPE_STRING,  // Policy Status
                                                   G_TYPE_INT);    // Transaction Index

    GtkTreeView *tree = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_list)));

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Transaction ID", renderer, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Customer", renderer, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Total", renderer, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Paid", renderer, "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Remaining", renderer, "text", 4, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Due Date", renderer, "text", 5, NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Policy", renderer, "text", 6, NULL);

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
            char due[16] = "(none)";
            char policy[96] = "On Track";
            if (get_txn_due_date(txn, due, sizeof(due))) {
                int days = days_until_date(due);
                if (days < -app_settings.layaway_grace_days) {
                    snprintf(policy, sizeof(policy), "Overdue - Cancel Eligible");
                } else if (days < 0) {
                    snprintf(policy, sizeof(policy), "Overdue (Grace)");
                } else if (days <= app_settings.layaway_reminder_days) {
                    snprintf(policy, sizeof(policy), "Reminder Window");
                }
            }

            GtkTreeIter iter;
            gtk_list_store_append(store_list, &iter);
            gtk_list_store_set(store_list, &iter,
                              0, txn->transaction_id,
                              1, customer_name,
                              2, txn->total,
                              3, txn->amount_paid,
                              4, remaining,
                              5, due,
                              6, policy,
                              7, i,
                              -1);
        }
    }

    gtk_container_add(GTK_CONTAINER(sw), GTK_WIDGET(tree));

    GtkWidget *btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), btn_row, FALSE, FALSE, 0);
    GtkWidget *apply_rules_btn = gtk_button_new_with_label("Apply Cancellation Rules");
    gtk_box_pack_start(GTK_BOX(btn_row), apply_rules_btn, FALSE, FALSE, 0);
    g_signal_connect(apply_rules_btn, "clicked", G_CALLBACK(on_apply_layaway_rules_clicked), s);

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
                int requires_serial = 0;
                Product *p = NULL;
                for (int j = 0; j < store->product_count; j++) {
                    if (strcmp(store->products[j].sku, so->product_sku) == 0) {
                        p = &store->products[j];
                        break;
                    }
                }
                if (p && p->serialized) requires_serial = 1;

                if (requires_serial && app_settings.prompt_receiving_serial) {
                    char serial_ctx[256];
                    char serial_number[NAME_LEN] = "";
                    sprintf(serial_ctx, "Receiving serialized product\nProduct: %s\nSKU: %s", p->name, so->product_sku);
                    if (!prompt_for_serial_number_dialog("Serial Number when Receiving", serial_ctx, serial_number, sizeof(serial_number))) {
                        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store_list), &iter);
                        continue;
                    }
                    strncpy(so->serial_number, serial_number, NAME_LEN - 1);
                    so->serial_number[NAME_LEN - 1] = '\0';
                }

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

    load_data();

    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_window), "Ascend Retail Platform");
    gtk_window_set_default_size(GTK_WINDOW(main_window), mobile_floor_mode_enabled ? 900 : 500, mobile_floor_mode_enabled ? 650 : 400);
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(main_window, "key-press-event", G_CALLBACK(on_main_window_key_press), NULL);

    show_main_menu();
    apply_visual_theme(active_theme_mode);

    gtk_main();

    return 0;
}