# 3-Minute Demo Video Script (Draft)

*Read-aloud pace assumed: ~2.5 words/second. Trim mid-sentence if you overshoot a section's budget — the running total in brackets is what matters, not hitting every line verbatim. Actions in *italics*, narration in plain text.*

---

### 0:00–0:20 — Code structure tour (20s)

*Have the IDE open on the file tree before recording starts.*

"This is a C++ trading platform built around five classes: `OrderBook` holds the market data and computes OHLC candles; `UserManager` handles login and registration; `Wallet` tracks balances; `Transaction` logs trades and drives the simulation; and `MerkelMain` is the per-login session that ties them together through the menus. Each class owns exactly one CSV file, so there's no overlap in who's allowed to touch what."

*[Running: 0:20]*

---

### 0:20–0:45 — Task 2: Login / Register (25s)

*Launch the program. Show the main menu.*

"Here's the main menu — Login/Register, or Reset password. I'll register a new user."

*Enter a fake name, email, password.*

"It validates the fields, checks for duplicates, and generates a unique 10-digit username — that's what I'll log in with next time. On my first login I get a signup bonus, seeded and saved to `USERS_WALLET.CSV` through the same code path every account uses — including the testuser you get by leaving the username blank."

*Log out, log back in with the generated username and password.*

"And logging back in compares the password against a stored hash — worth noting, `std::hash` isn't cryptographically secure, which I discuss in the report."

*[Running: 0:45]*

---

### 0:45–1:10 — Task 1: OHLC summary (25s)

*Navigate to Market -> Print OHLC summary.*

"Under Market, I can pull an OHLC summary for a product — by default it's the yearly view, computed in one pass over the order data: first price seen is open, last is close, running min and max give low and high."

*Show the annual table, then opt into the date filter.*

"And I can optionally filter down to a specific date range for a daily breakdown — or generate the missing market-history years right here, which I'll show in the simulation section."

*[Running: 1:10]*

---

### 1:10–1:40 — Task 3: Wallet & transactions (30s)

*Navigate to Wallet.*

"The Wallet menu shows my balance automatically, and lets me deposit or withdraw."

*Deposit some currency.*

"That updates the balance and persists it immediately."

*Show recent transactions and activity statistics.*

"Recent transactions shows my last five trades; activity statistics gives ask/bid counts per product and total spend, optionally filtered by date — both read straight from `USERS_TRADING.CSV`."

*[Running: 1:40]*

---

### 1:40–2:30 — Task 4: Simulation (50s)

*Navigate to Market -> Print OHLC summary, pick a product, choose Generate.*

"First, market history. The Generate option inside the OHLC view backfills every missing year between the dataset's 2020 data and today — including the current year up to this moment — one year at a time, then re-prints the summary so the new years show immediately. It reuses the exact same transaction generator as the real simulation, under a system identity whose trades count as market data only — they never touch my wallet or trading history."

*Navigate to Wallet -> Simulation, run Start.*

"Now Start runs my own simulation, every order stamped at the real current time. You can see each order's fulfilment check printed live — filled, or skipped for insufficient funds. The price model is a seed price times a per-product drift, compounded by year, plus random noise — and my fills cross the one-percent spread like a real market taker, selling at the bid and buying at the ask, so trading costs money and skips genuinely happen."

*Show the final "X filled, Y skipped" summary, then USERS_TRADING.CSV.*

"It batches until at least fifty transactions have actually filled, the wallet file is saved after every single fill, and skipped orders are recorded in the trading history too, with their own status."

*[Running: 2:30]*

---

### 2:30–2:55 — Task 5: Input validation (25s)

*Trigger a couple of bad inputs quickly: a non-numeric menu choice, an unknown product, a reversed date range.*

"Every menu choice goes through one shared, validated input helper — bad input never crashes the program, it just re-prompts. Same pattern everywhere else: unknown products, malformed dates, non-numeric amounts are all caught and rejected before they can do anything."

"I also removed the template-only manual ask, manual bid, and continue-to-next-timestamp options, because the assignment's trading activity is handled by the required simulation and wallet history instead."

*[Running: 2:55]*

---

### 2:55–3:00 — Close (5s)

"That covers all five tasks — data processing, authentication, wallet and transaction history, simulation, and input validation, all built around small, single-purpose classes."

*[Running: 3:00]*
