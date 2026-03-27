# Ascend-Clone

An open-source retail point-of-sale platform inspired by Ascend, built for small bike shops and specialty retailers.

## Features

- Multi-store management
- Customer database
- Inventory & product tracking
- Sales transactions with payment processing (cash, credit, debit, gift card)
- Layaway creation and tracking
- Special orders with status tracking (not ordered → on order → received → complete)
- Reorder list / buyer's report
- Special order notifications
- Complete-a-Sale utility (F10)
- Quotes
- Business reports & daily sales trends
- Data persistence (save/load)

## Requirements

- macOS or Linux
- GTK+ 3 (`brew install gtk+3` on macOS)
- GCC or Clang with C11 support

## Build & Run

```bash
make        # build
make run    # build and launch
make clean  # remove binary and data file
```

Or manually:

```bash
gcc -std=c11 -Wall -o ascend main_gtk.c $(pkg-config --cflags --libs gtk+-3.0)
./ascend
```

## Data

All store data is saved to `ascend_data.txt` in the working directory automatically when you click **Save** or perform certain actions.

