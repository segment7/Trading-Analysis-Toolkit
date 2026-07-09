# CM2005 Object Oriented Programming — Midterm Report (Draft)

*Draft assembled from `validation-notes.md`'s design-decision log. Replace every `[SCREENSHOT: ...]` placeholder with an actual screenshot before submitting. Prose can be tightened/personalised — this draft prioritises completeness over polish.*

## Environment

- C++ standard: C++17
- Compiler: g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
- OS: Ubuntu 24.04.3 LTS (WSL2 on Windows)
- Build command: `g++ -std=c++17 -Wall *.cpp -o mr` (no IDE build system; VS Code used as editor)

## Class structure overview

`OrderBook` + `Candlestick` (market data, read-only, OHLC) · `UserManager` (register/login/reset, owns `USERS_REGISTER.CSV`) · `Wallet` (balances, owns `USERS_WALLET.CSV`) · `Transaction` (history + simulation generator, owns `USERS_TRADING.CSV`) · `Simulation` (Generate/Start wrapper around `Transaction`) · `MerkelMain` (per-login session, menus/IO only) · `Input` (shared validated-input helpers) · `CSVReader`/`OrderBookEntry` (template baseline, low-level parsing).

Single-responsibility split: each manager class owns exactly one CSV file and nothing outside its own concern touches that file directly.

---

## Task 1: Loading Trading Data and Computing OHLC Data

**(i) Explanation**

The market CSV (`20200601.csv`) is loaded once into `OrderBook` at program start (`main.cpp`) and shared by reference across every logged-in session — never reloaded, never modified. The user enters a product (e.g. `ETH/USDT`); by default `printOHLC` shows the yearly summary for both asks and bids; follow-up options let the user "Filter" down to a daily breakdown between two dates, or "Generate" the missing market-history years (Task 4's backfill) and immediately see the refreshed multi-year summary.

**(ii) Implementation logic**

`OrderBook::computeOHLC` groups already-filtered, chronologically-ordered entries by a timestamp-prefix key (4 chars = year, 10 chars = full date) in a single pass — first entry seen in a group is `open`, last is `close`, running min/max give `low`/`high`. Deliberately not built on top of the template's `getHighPrice`/`getLowPrice` helpers, since that would require a separate vector and an extra pass per group over what can be a very large order file.

```
for each entry in date-filtered, time-ordered orders:
    key = timestamp prefix (year or full date)
    if key is new:
        start a new candle: open = high = low = close = price
    else:
        update this candle's high/low/close
return list of candles
```

Two comparators for asks/bids are shown together on every call (`printOHLC`'s `printTable` lambda, parameterised by `OrderBookType`) rather than asking the user to pick a side — task.md phrases both tables as the expected output, not a menu choice.

`OrderBook::getOrdersInRange` and the plain-timestamp `getOrders` share one private `filterOrders(predicate)` core (`std::copy_if` + lambda) — a small DRY refactor once the duplication between the two was noticed.

**(iii) Screenshot**

`[SCREENSHOT: yearly OHLC table for a product, multi-year after Task 4 backfill]`
`[SCREENSHOT: date-range-filtered daily OHLC table]`

---

## Task 2: Login or Register a Trading User Profile

**(i) Explanation**

One combined entry point (`UserManager::loginOrRegister`) rather than separate menu items: the user types a username; a blank username logs them in as the reserved **testuser `0000000000`** — a real, persisted account whose only special property is that it bypasses credentials for quick testing. It cannot collide with a registered account because generated usernames start at 1000000000. A non-blank username is looked up first — if not found, the password prompt is skipped entirely and the user goes straight to a y/n registration offer; if found, a password prompt follows and is compared against the stored hash.

**(ii) Implementation logic**

```
username = prompt "Username (blank for testuser)"
if username is blank: return TEST_USERNAME ("0000000000")
look up username in USERS_REGISTER.CSV
if not found:
    ask "Register as new user? (y/n)"
    if yes: run registration, log the new user straight in
    else: fail
else:
    password = prompt
    if hash(password) == stored hash: success
    else: "Login failed." (generic — no detail on which field was wrong)
```

Registration (`registerUser`) validates all three fields non-empty, rejects commas (would corrupt the CSV row), requires an `@` in the email, and checks full-name+email against existing rows before inserting (task.md 2.1.5, duplicate prevention). A unique 10-digit username is generated with `std::mt19937_64` + `uniform_int_distribution`, re-rolled on collision. The password is stored as `std::to_string(std::hash<std::string>{}(password))` — **note for discussion:** `std::hash` is explicitly not a cryptographic hash (no salting, vulnerable to precomputation/collision attacks); it's used here because task.md mandates it, but a real system would need a salted cryptographic hash (e.g. bcrypt/argon2). This exact gap is the anchor point for the SDD Part 3 report's cryptographic-practice OWASP item.

Signup bonus: 200 BTC, 200 ETH and 200 USDT are granted on **first login** rather than at registration — the `MerkelMain` constructor loads the wallet from `USERS_WALLET.CSV` and, if it comes back empty, seeds and persists the bonus (creating the file if this is the first account). One code path serves both freshly registered users and the testuser (DRY: the bonus logic exists exactly once).

**(iii) Screenshot**

`[SCREENSHOT: register flow showing the generated 10-digit username]`
`[SCREENSHOT: successful login]`
`[SCREENSHOT: failed login / unknown username -> registration offer]`
`[SCREENSHOT: USERS_REGISTER.CSV contents]`

---

## Task 3: User Wallet & Trading Transactions History

**(i) Explanation**

`Wallet` owns `USERS_WALLET.CSV` (one row per `username,currency,amount`); it's loaded on login and saved after **every** balance change — deposits, withdrawals, and each individual settled simulation trade — so a crash mid-run can never leave the file behind the logged trades. Deposit/withdraw are two menu actions under Wallet. `Transaction` owns `USERS_TRADING.CSV` and is the single place that appends a trade row after a simulation fill or skipped attempt. Filled rows and wallet updates happen together, and orders rejected for insufficient balance are recorded with a `skipped` status, so the trading history shows attempts as well as outcomes. Wallet also auto-displays the current balance on entry to the Wallet menu and after every wallet-affecting action, so a separate "show balance" option was removed as redundant.

**(ii) Implementation logic**

```
deposit/withdraw:
    read currency (non-empty) and amount (std::stod, must be > 0)
    withdraw additionally checked against Wallet::containsCurrency
    on success: update in-memory balance, persist to USERS_WALLET.CSV

logTransaction(username, sale, status = "filled"):
    append one row: username,timestamp,product,ask/bid,price,amount,status
    (status = "filled" for settled trades, "skipped" for insufficient balance)
```

Recent transactions (last 5, with their status) and activity statistics (ask/bid counts per product + total spent in a date range, plus a separate skipped count — skipped rows are excluded from the settled totals since they never traded) both read `USERS_TRADING.CSV` filtered to the current user; the activity-stats date filter reuses the exact same validation convention as Task 1's OHLC date inputs, for consistency.

**(iii) Screenshot**

`[SCREENSHOT: deposit and withdraw actions with balance updating]`
`[SCREENSHOT: last 5 transactions]`
`[SCREENSHOT: activity statistics, all-time and date-filtered]`
`[SCREENSHOT: USERS_WALLET.CSV and USERS_TRADING.CSV contents before/after a trade]`

---

## Task 4: Simulating User Trading Activities

**(i) Explanation**

`Simulation` wraps `Transaction::generateTransactions` (the single reusable generator used for every simulated trade, real or backfilled) behind two features: **Generate** (the OHLC summary's follow-up option) backfills any market-history years missing between the dataset's own year (2020) and today — *including* the current year, capped at the present moment — under a non-persisted `system` identity, then re-prints the yearly summary so the new years are immediately visible; it lives inside the OHLC view because it's really seeding OHLC history rather than the user's own trading. **Start** (under Wallet → Simulation) runs the logged-in user's own graded simulation, with every order stamped at the real current wall-clock time.

**(ii) Implementation logic — price model**

```
price = seed * (1 + drift)^(year - 2020) * (1 + noise)
seed  = last real ask price for this product from the 2020 dataset
drift = fixed per-product constant (e.g. +8%/yr BTC/USDT, -3%/yr DOGE/BTC)
noise = uniform random ±5% per order
book ask = price * 1.01   (resting order, feeds ask-side OHLC)
book bid = price * 0.99   (resting order, feeds bid-side OHLC)
user's fill crosses the spread, like a real market taker:
    sell settles at price * 0.99 (hits the best bid)
    buy  settles at price * 1.01 (lifts the best ask)
```

Stateless — no price is carried across years or calls; years still differ meaningfully thanks to the fixed drift term. No prediction algorithm is used or required (task.md 4.2.i). The spread-crossing detail matters for realism: an earlier version settled the user's fills on the favourable side (sell high, buy low), which meant every round trip *earned* ~2% and the wallet grew without bound — no order was ever skipped. Crossing the spread makes simulated trading cost money, so insufficient-balance skips genuinely occur.

**(ii) Implementation logic — date gap (task.md 4.2.ii)**

The provided dataset only contains 2020 records; the current system year is several years ahead. Rather than a separate mechanism, `Simulation::init()` calls the *same* `generateTransactions` function used for the user's own trading, once per missing year (2021 through the current year — the generator caps current-year timestamps at "now"), under a `system` identity with a throwaway, generously-funded wallet. Persistence is controlled by an explicit `persist=false` argument rather than a magic username check: gap-fill trades are market data only, never written to the user trading log or wallet file. This is deliberately reused, not reimplemented, as evidence of DRY/reusability. Idempotent: a year that already has data is skipped, so re-running Generate does no extra work.

**(ii) Implementation logic — guaranteeing "at least 50" and insufficient balance (task.md 4.1, 4.2.iii)**

The literal requirement ("5 ask + 5 bid per product") only reaches 50 total because the shipped dataset happens to have exactly 5 products. `Simulation::start()` instead computes `ordersPerSide` from the *live* product count (`ceil(50 / (productCount * 2))`), then loops in batches (up to 10) until at least 50 orders have actually **filled** — so skips don't silently shrink the run below the required volume. Each attempted order is checked via `Wallet::canFulfillOrder` against its actual settlement price before it's allowed to settle — insufficient balance is skipped and counted (task.md 4.2.iii), not allowed through — and every check prints `canFulfillOrder [ask/bid] ... filled/skipped` to the console, giving visible, per-order evidence. Settled fills update the wallet (`Wallet::processSale`), are logged with `filled` status, and the wallet file is saved immediately per fill; skipped orders are logged with `skipped` status.

```
productCount = number of distinct products in the order book
ordersPerSide = ceil(50 / (productCount * 2))
repeat (max 10 batches) until totalFilled >= 50:
    for each product:
        generate ordersPerSide asks + ordersPerSide bids,
            each stamped with the real current wall-clock time
        each order: check wallet can afford the settlement
            -> settle + log "filled" + save wallet file, else log "skipped" and count it
```

**(ii) Implementation logic — template cleanup decision**

The original MerkelRex template included manual "make ask", "make bid" and "continue to next timestamp" options, plus an exact-timestamp matching engine. Those paths are not required by the assignment tasks: Task 4 asks for simulated trading transactions, and Task 3 asks for transaction history and wallet persistence. Keeping the template manual trading flow would add another settlement path to validate and explain, so it was removed from the visible menu and the unused supporting methods were deleted. The submitted program now has one clear user-trading path: `Simulation::start()` generates the required ask/bid activity, checks the wallet, logs each result, and persists the wallet immediately after every fill.

**(iii) Screenshot**

`[SCREENSHOT: Generate (OHLC follow-up) running, per-year progress lines + refreshed yearly table]`
`[SCREENSHOT: Start running, "canFulfillOrder" trace + final "X filled, Y skipped" summary]`
`[SCREENSHOT: wallet balance before/after simulation + USERS_TRADING.CSV rows with filled/skipped status]`
`[SCREENSHOT: multi-year OHLC proving the backfilled years are visible in Task 1]`

---

## Task 5: Printing User Interaction Menu

**(i) Explanation**

Every menu (pre-login, logged-in, Market, Wallet, Simulation) follows one convention: numbered options, `0` always means exit/logout/back, and every choice is read through one shared `Input::promptChoice(min, max)` helper — loops until a valid in-range integer is entered, catches non-numeric input via `std::stoi`'s exception, and treats EOF as a clean exit (so piped/scripted test input never hangs). This one helper is reused by every menu in the program rather than each menu re-implementing its own input loop.

**(ii) Implementation logic**

```
promptChoice(min, max):
    loop:
        print "Type in <min>-<max>: "
        read a line; if EOF, return 0 (exit/back)
        try: parse as int
        if parse succeeds and min <= value <= max: return value
        else: print "Invalid choice." and loop again
```

All free-text inputs elsewhere in the program (product names, dates, currency codes, deposit/withdraw amounts) are validated at their point of use before being acted on — e.g. OHLC product names are checked against `getKnownProducts()`, dates are checked for blank-or-10-characters and a non-reversed range, and deposit/withdraw amounts are parsed with `std::stod` in a try/catch and rejected if non-numeric or ≤ 0. As a second layer, `Wallet::canFulfillOrder`/`processSale` fail securely on any product that doesn't split into exactly `BASE/QUOTE`, protecting the simulation and any future trading path from malformed product strings. None of these can crash the program; all fall back to a clear message and return to the menu.

**(iii) Screenshot**

`[SCREENSHOT: invalid menu choice (letters) rejected, re-prompts]`
`[SCREENSHOT: unknown product rejected in OHLC]`
`[SCREENSHOT: invalid/reversed date range rejected]`
`[SCREENSHOT: invalid deposit/withdraw amount rejected]`

---

## Code style & OOP evidence summary

- **Encapsulation:** each manager class (`UserManager`, `Wallet`, `Transaction`) owns exactly one CSV file; nothing outside it touches that file.
- **Reusability:** `Transaction::generateTransactions` serves three roles from one function — the user's own "Start" simulation, the historical backfill behind OHLC's Generate option, and (indirectly) the multi-year data behind Task 1's OHLC — parameterised by `ordersPerSide`, `liveTimestamps` (real wall-clock time vs random moments in a year) and `persist` (user activity vs market-data-only), so policy is stated explicitly at each call site instead of inferred from a magic username.
- **DRY:** `OrderBook::filterOrders` is the shared core behind the date-range queries and seed-price lookup; `Input::promptChoice` is the single validated-choice helper behind every menu in the program; `OrderBook::DATASET_YEAR` is the single source of truth for the dataset's year, replacing what were previously two independent hardcoded `2020` literals.
- **Single Responsibility:** `MerkelMain` only handles menus/IO and delegates everything else; `Wallet` only handles balances; `OrderBook` only handles market data and OHLC.
- **Template cleanup verified, not assumed:** every declared function across the header files was checked for a real call site. The unnecessary template manual-trading path was removed from both menu and code: `enterAsk`, `enterBid`, `gotoNextTimeframe`, `OrderBook::getOrders`, `OrderBook::matchAsksToBids`, price comparators, and the manual CSV conversion overload are no longer part of the submission.
