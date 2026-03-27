# Ascend-Clone Program Instructions

This guide is a full operating manual for the current Ascend-Clone build.
It is designed for:
- Day-to-day cashier use
- Store manager workflows
- Quick troubleshooting
- Fast feature discovery with keyword search

---

## How to Use This Page Fast

Use your editor/browser search (`Cmd+F`) with:
- Menu paths, like `View > Time Clock`
- Feature names, like `Layaway`, `Special Orders`, `Returns`
- Search tags, like `#sales`, `#inventory`, `#report-timeclock`
- Keywords from the A–Z index at the bottom

---

## Quick Navigation (Feature Map)

- `Desktop Tile: Add Store` -> Create a store
- `Desktop Tile: Inventory` -> Add/edit products
- `Desktop Tile: Customers` -> Manage customer records
- `Desktop Tile: Sales` -> Standard sales workflow
- `Desktop Tile: Return` -> Return/refund workflow
- `Desktop Tile: Layaway` -> Open/partial-pay transactions
- `View > Layaways` -> Review open layaways
- `View > Special Ordered Items` -> View all special orders
- `View > Customer Tax Exceptions` -> Add/manage tax exception reasons
- `View > Time Clock` -> Add/modify/remove/restore employee time entries
- `Options > Sales and Returns > Work Order Defaults` -> Serial prompt setting (work-order default)
- `Options > Ordering` -> Serial prompt setting for receiving
- `Reports > Reorder List` -> Buyer list for un-ordered special orders
- `Reports > Special Order Items On Order` -> Items currently on order
- `Reports > Special Order Items Not Yet Received` -> Backlog visibility
- `Reports > Special Order Items Received` -> Receiving queue + notifications
- `Reports > Time Clock` -> Time report with employee/date filters + CSV export
- `Reports > Daily Sales + Payment Breakdown` -> Revenue/payment summary + CSV export
- `Reports > Low Stock` -> Threshold-based stock warnings + CSV export
- `Reports > Best-Selling Items` -> Top movers

---

## Quick Reference Cards

## 1) Run the Program

- Build: `make`
- Run: `make run`
- Clean build artifacts: `make clean`

## 2) Daily Open Checklist

- Load latest data (`Load Data` tile)
- Verify store selected in each workflow
- Check:
  - `Reports > Low Stock`
  - `Reports > Special Order Items Received`
  - `View > Time Clock` for missed clock-outs

## 3) End-of-Day Checklist

- Complete pending sales/layaways
- Mark received special orders and send notifications
- Run reports:
  - Daily Sales + Payment Breakdown
  - Time Clock
- Export CSVs needed for accounting/payroll
- Save data (`Save Data` tile)

---

## Detailed Feature Instructions

## Desktop and Tiles  #desktop #tiles

- Left click tile while desktop is locked -> execute feature
- Right click toggles desktop lock/unlock mode (drag tiles when unlocked)

Core tiles available:
- Add Store
- List Stores
- Store Info
- Inventory
- Customers
- Sales
- Return
- Layaway
- Business
- Trend Chart
- Save Data
- Load Data
- Exit

---

## Store Management  #store #multi-store

### Add Store

1. Click `Add Store` tile
2. Enter:
   - Store Name
   - Location
   - Monthly Goal
3. Save

### Store Selection

Most workflows prompt for store first.
Always verify you are operating in the intended store before continuing.

---

## Inventory Management  #inventory #products #sku #barcode

### Add/Edit Product

Available fields include:
- SKU
- Name
- Vendor
- Serialized flag
- Price
- Stock

### Stock and Sales Relationship

- Sales reduce stock when transactions close
- Returns re-add stock
- Layaways can remain open depending on payment flow

### Low Stock Monitoring

Use `Reports > Low Stock` and set threshold.
Export to CSV when needed.

---

## Customer Management  #customers #crm #store-credit

Customer records include:
- Name/company
- Contact details
- Billing/shipping
- Tax ID
- Notes
- Account metadata (POA, credit limit)

Used by:
- Sales
- Returns
- Special orders
- Notifications

---

## Sales (Standard POS)  #sales #pos #payments #receipt

### Create Sale

1. Click `Sales` tile
2. Choose store
3. Select customer
4. Add products by SKU/UPC + quantity
5. Take payment
6. Optionally print receipt

### Payment Types

- Cash
- Credit Card
- Debit Card
- Gift Card / Store credit equivalent flow

### Important Notes

- Serialized products require serial capture at time of sale
- This sale-time serial association is intentionally mandatory

---

## Returns  #returns #refunds

### Create Return

1. Click `Return` tile or `Sales > Create a Return`
2. Use `Previous Sales Items` flow when receipt is available
3. Select returned items
4. Confirm original customer crediting (Yes/No)
5. Process refund method

Behavior:
- Return items use negative quantity/negative balance logic
- Inventory is increased by returned quantity
- Return transaction stored with original transaction reference

---

## Layaways  #layaway #open-transactions

### Create Layaway

1. Click `Layaway` tile
2. Select customer
3. Add items
4. Save layaway or apply partial payment

### View Layaways

- `View > Layaways`
- Shows transaction ID, customer, total, paid, remaining

### Complete Open Sale Utility

- `F10` key opens Complete-a-Sale utility
- Search open transaction by ID or customer
- Complete transaction and close it

---

## Special Orders  #special-orders #reorder #receiving

### Creation

When stock is insufficient during sale/layaway add-item, special order options appear.

### Statuses

- Not Yet Ordered
- On Order
- Received
- Complete

### Reports and Actions

- `Reports > Reorder List`
  - Select and mark items as ordered
- `Reports > Special Order Items On Order`
- `Reports > Special Order Items Not Yet Received`
- `Reports > Special Order Items Received`
  - Mark as received
  - Send notification to customer

### Receiving + Serial Prompt Behavior

If item is serialized and receiving prompt setting is enabled:
- Marking special order as received requests serial number entry

---

## Serial Number Prompt Settings  #serial #settings #options

### Work Order Prompt Setting

Path:
- `Options > Sales and Returns > Work Order Defaults`

Setting:
- `Prompt for Serial Number`

### Receiving Prompt Setting

Path:
- `Options > Ordering`

Setting:
- `Prompt for Serial Number when Receiving`

### Cannot Be Disabled

- Sale-time serial association prompt for serialized items remains required by design.

---

## Customer Tax Exceptions  #tax #exceptions

Path:
- `View > Customer Tax Exceptions`

Add flow:
1. Click `Add`
2. Enter Description
3. Set `Requires Tax ID` as needed
4. Keep `Hidden` unchecked to keep active
5. Save

---

## Time Clock  #time-clock #employees #hours #payroll

Path:
- `View > Time Clock`

### Add Entry

1. Click `Add`
2. Select/type User
3. Enter Start Time (`YYYY-MM-DD HH:MM`)
4. Check/uncheck End Time
5. Save

### Modify Entry

1. Select row
2. Click `Modify`
3. Edit values
4. Save

### Remove/Restore

- Remove marks entry hidden
- Restore un-hides selected entry
- Use `Show Hidden` filter to view/restorable entries

### Date Filtering

- Use `From` and `To` in Time Clock view
- Click `Refresh`

---

## Time Clock Report  #report-timeclock #csv

Path:
- `Reports > Time Clock`

Supports:
- Date range filter
- Employee filter (`All Employees` or specific user)
- Refresh
- CSV export

Highlights:
- `ALERT: Long shift` for shifts >= 10 hours
- `ALERT: Missing clock-out` when no end time

CSV output file:
- `time_clock_report.csv`

Columns:
- User
- Start Time
- End Time
- Hours
- Alert

---

## Daily Sales + Payment Breakdown Report  #report-sales #csv #finance

Path:
- `Reports > Daily Sales + Payment Breakdown`

Tracks by date range:
- Transaction count
- Revenue total
- Payment breakdown:
  - Cash
  - Credit
  - Debit
  - Gift/Store-credit style flow

CSV output file:
- `daily_sales_payment_report.csv`

---

## Low Stock Report  #report-inventory #csv #alerts

Path:
- `Reports > Low Stock`

Inputs:
- Threshold value

Output:
- Products at/below threshold
- Includes SKU, stock, and price

CSV output file:
- `low_stock_report.csv`

---

## Best-Selling Items Report  #report-bestsellers #analytics

Path:
- `Reports > Best-Selling Items`

Output:
- Top sold products sorted by units sold
- Includes SKU and current stock

---

## Trend Chart  #trend #analytics

Path:
- `Trend Chart` tile

Shows daily sales trend visualization for selected store.

---

## Data Save/Load and Files  #data #backup

### Save / Load

- Use `Save Data` tile and `Load Data` tile
- Some workflows auto-save after key operations

### Main data file

- `ascend_data.txt`

### Generated export files

- `time_clock_report.csv`
- `daily_sales_payment_report.csv`
- `low_stock_report.csv`

---

## Troubleshooting Quick Fixes  #troubleshooting

### "Product not found"
- Verify SKU spelling/scanning
- Confirm product exists in selected store

### No rows in report
- Check date range
- Check selected store
- For Time Clock, verify employee filter is `All Employees` or correct user

### Cannot restore removed time clock row
- Enable `Show Hidden`
- Refresh list
- Select row and click `Restore`

### Serial prompt confusion
- Receiving prompt can be toggled in `Options > Ordering`
- Sale-time prompt for serialized products is mandatory

---

## Role Notes (Current Build)  #permissions

There is currently no fully enforced role-based permission layer in this build.
Some settings are presented manager-style, but hard permission checks are limited.

---

## Search Tags (Copy/Paste)

`#desktop #tiles #store #inventory #products #sku #barcode #customers #sales #pos #payments #receipt #returns #refunds #layaway #open-transactions #special-orders #reorder #receiving #serial #settings #options #tax #exceptions #time-clock #employees #hours #payroll #report-timeclock #report-sales #report-inventory #report-bestsellers #analytics #trend #data #backup #csv #finance #alerts #troubleshooting #permissions`

---

## A–Z Keyword Index

A
- Add customer
- Add product
- Add store
- Add time clock entry
- Alerts

B
- Barcode lookup
- Best-Selling Items report
- Business reports

C
- Cash payment
- Complete-a-Sale (F10)
- Credit payment
- Customer Tax Exceptions
- CSV export

D
- Daily Sales + Payment Breakdown
- Data save/load
- Debit payment
- Desktop tiles
- Discounts (see sale flow extension points)

E
- Employee filter (time clock report)
- End Time
- Export CSV

F
- F10 complete sale

G
- Gift card payment

H
- Hidden time clock entries
- Hours worked

I
- Inventory
- In-stock quantity

K
- Keywords/search tags

L
- Layaway
- List Stores
- Load Data
- Low Stock report

M
- Mark as received
- Missing clock-out
- Modify time clock entry

N
- Notifications (special orders)

O
- On Order report
- Options > Ordering
- Options > Sales and Returns

P
- Payment breakdown
- PO / special-order pathways
- Product serialized flag

R
- Reorder List
- Reports menu
- Restore time clock entry
- Returns

S
- Sales
- Save Data
- Serial number prompts
- Special orders
- Store credit style (gift flow)

T
- Tax exceptions
- Time Clock
- Time Clock report
- Trend chart

U
- User filter

V
- View menu

W
- Work Order Defaults (serial prompt setting)

---

## Version Note

This page documents the current codebase behavior in `main_gtk.c` as of this update.
If features change, update this file so staff always has a trusted operations guide.
