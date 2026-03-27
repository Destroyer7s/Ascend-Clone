# Ascend Clone - 5 New Features Implementation Guide

## Overview
This guide outlines the 5 major features you requested and their implementation status:

### ✅ Feature 1: Custom Font & Color Picker
**Status**: Data structures ready  
**Implementation**: Already added to `AppSettings` struct with fields:
- `custom_font_name` - Font family (e.g., "Ubuntu Mono")
- `custom_font_size` - Size in points  
- `ui_color_primary` - Primary UI color (hex format)
- `ui_color_secondary` - Secondary UI color
- `ui_color_text` - Text color

**Where to integrate**: Add menu item in Tools menu → "Font & Color Settings"

**Implementation code template**:
```c
static void font_color_picker_dialog(void) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Font & Color Settings",
                                                   GTK_WINDOW(main_window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Apply", GTK_RESPONSE_OK,
                                                   "_Cancel", GTK_RESPONSE_CANCEL, NULL);
    // Font chooser
    GtkWidget *font_btn = gtk_font_button_new_with_font(app_settings.custom_font_name);
    gtk_box_pack_start(...);
    
    // Color choosers for primary, secondary, text
    GtkWidget *primary_color = gtk_color_button_new();
    GtkWidget *secondary_color = gtk_color_button_new();
    GtkWidget *text_color = gtk_color_button_new();
    
    if (gtk_dialog_run(...) == GTK_RESPONSE_OK) {
        // Update app_settings and apply_visual_theme()
        save_data();
    }
}
```

---

### ✅ Feature 2: Employee Account Management & Role-Based Permissions
**Status**: Data structures 100% ready  
**Global arrays**: `employees[]`, `employee_count`, `next_employee_id`  
**Role enum**: `EmployeeRole` with BASIC, CASHIER, MANAGER, ADMIN, SERVICE_LEAD, BUYER

**What's already implemented**:
- EmployeeAccount struct with full permission flags
- Role-based access control functions: `role_can_manage()`, `role_can_service()`, `role_can_inventory()`, etc.
- Security override PIN system
- Manager override request dialog  
- Audit logging for all role changes

**Integration needed**: Add to Tools menu → "Manage Employees"

**Implementation outline**:
1. Employee CRUD dialog (add/edit/delete accounts)
2. Permission matrix editor (checkboxes for each permission per role)
3. Audit log viewer for employee actions
4. Login/role-switch interface

**Key permission levels already coded**:
- ADMIN: Full system access, SQL queries, employee management
- MANAGER: Inventory, customer management, report creation
- CASHIER: Process sales/returns only
- SERVICE_LEAD: Queue prioritization, repair scheduling
- BUYER: Procurement dashboard, margin comparisons
- BASIC: View-only

---

### ⚠️ Feature 3: Graphical Calendar Widget
**Status**: Data structures ready for scheduling  
**Related structs**:
- `RepairSchedule` - tracks service work assignments
- `EmployeeSchedule` - tracks shift assignments

**What needs implementation**:
1. GTK calendar widget that displays:
   - Repair jobs (color-coded by status: pending, in-progress, completed)
   - Employee shifts (color-coded by employee)
   - Clickable events to edit/view details

2. Month/week view selector
3. Drag-and-drop reassignment for repairs
4. Conflict checking (employee booked twice, repair overdue)

**Integration**: Tools menu → "Scheduling Calendar"

**GTK Calendar approach**:
```gdkcalendar
GtkWidget *calendar = gtk_calendar_new();
g_signal_connect(calendar, "day-selected", G_CALLBACK(on_calendar_day_selected), dialog);

// Overlay repair/schedule events on selected day
// Use gtk_widget_set_tooltip_text() to show event details
```

---

### ⚠️ Feature 4: Repair/Schedule Visualization 
**Status**: Data & functions partially implemented

**What's ready**:
- `RepairSchedule` struct with all tracking fields
- `repair_schedules[]` global array
- Repair status enum: WO_OPEN, WO_WAITING_PARTS, WO_IN_PROGRESS, WO_COMPLETED, WO_PICKED_UP
- Service stand manager already implemented
- Work order workflows in place

**What needs implementation**:
1. **Timeline view**: Horizontal Gantt chart showing:
   - Repair ID | Customer | Scheduled Date/Time → Estimated Completion
   - Color coding by status
   - Click to edit/update progress

2. **Service board view**: 
   - Grid: stands (rows) × time slots (columns)
   - Show which mechanic is assigned to each stand
   - Drag repairs between stands

3. **Real-time status updates**:
   - SMS notifications to customers when work transitions states
   - Estimated completion time calculator

**Integration**: Tools menu → "Repair Timeline" + "Service Board"

---

### ⚠️ Feature 5: Web Scraper for bikeworldiowa.com
**Status**: Scraper framework + search system ready

**What's implemented**:
- `ScannedProduct` struct for storing scraped inventory
- `scanned_products[]` global array (holds up to 12,000 items)
- `qbp_catalog[]` for QBP distributor data (similar structure)
- Product lookup dialog with search by SKU/UPC/model
- Vendor integration hub already has QBP sync stub

**What needs implementation**:

1. **Scraper utility** (separate tool or tool integration):
```bash
# Option A: Python scraper script
python scraper.py --url https://www.bikeworldiowa.com --output scanned_products.json

# Option B: C libcurl implementation within main_gtk.c
```

2. **Scraper logic**:
- Parse bikeworldiowa.com product catalog pages
- Extract: SKU, product name, category, vendor, price, URL, stock status
- Handle pagination (categories × pages)
- Update `scanned_products` array with `last_scanned` timestamp

3. **Integration into Ascend**:
- Tools menu → "Refresh Web Product Catalog"
- Automatic daily refresh (with background task)
- Add scraped products to quote/purchase workflows
- Search bar filters by: name, category, vendor, price range

4. **Related functions already in place**:
- `product_lookup_dialog()` - searches both local + QBP + web catalogs
- `vendor_integration_hub_dialog()` - entry point for all catalog syncs

---

## Implementation Priority

**Quick wins (1-2 hours each)**:
1. Font & Color Picker (Settings dialog)
2. Employee CRUD UI (add/edit/remove accounts)

**Medium complexity (2-4 hours)**:
3. Simple calendar view (GTK calendar + repair overlay)
4. Web scraper stub (mock data or basic HTML parsing)

**Advanced (4+ hours)**:
5. Repair timeline Gantt chart
6. Service board visualization
7. Full web scraper with real bikeworldiowa.com integration

---

## Code Integration Checklist

- [ ] Add `font_color_picker_dialog()` function
- [ ] Add menu item in Tools: `g_signal_connect(..., "activate", G_CALLBACK(font_color_picker_dialog), NULL);`
- [ ] Create `manage_employees_dialog()` with CRUD operations
- [ ] Create `employee_permissions_matrix_dialog()` 
- [ ] Add calendar visualization (`scheduling_calendar_dialog()`)
- [ ] Add repair timeline (`repair_timeline_dialog()`)
- [ ] Add web scraper tool or Python integration (`web_scraper_refresh_dialog()`)
- [ ] Integrate all to main menu bar
- [ ] Update save/load functions to persist new employee accounts and schedules
- [ ] Test role-based access on all protected operations

---

## File Structure Notes

**Main file**: `main_gtk.c` (currently ~9500+ lines)

**Already in place**:
- Employee/role infrastructure
- Repair/schedule structs  
- Product scanning structs
- Calendar integration stubs
- Vendor hub entry point

**Ready for contribution**: All 5 features can be implemented by:
1. Creating dialog functions above the `main_gtk.c` function definitions
2. Adding menu items to `show_main_menu()`
3. Ensuring role checks before sensitive operations
4. Calling `save_data()`/`load_data()` for persistence

---

## Next Steps

1. **Start with Font & Color**: Simplest (just color pickers + apply styling)
2. **Then Employee Management**: Core feature that gates other features
3. **Calendar**: Adds visual appeal & schedule management
4. **Web Scraper**: Practical feature for quote building
5. **Repair Timeline**: Advanced visualization for service teams

All groundwork is complete. These are primarily UI/integration layer additions! 🚀
