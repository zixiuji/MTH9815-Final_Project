## Description

This final project for **MTH_9815** is centered on building a **bond trading system** for seven U.S. Treasury securities: **2Y, 3Y, 5Y, 7Y, 10Y, 20Y,** and **30Y**. Each security has its own CUSIP, coupon rate, and maturity date, and uses **“T”** as its ticker. The system leverages new definitions in `soa.hpp`—specifically, the concepts of **Service**, **ServiceListener**, and **Connector**:

- **Service**: Manages a particular type of data (e.g., pricing, market data, executions) keyed by an identifier.
- **ServiceListener**: Reacts to events when data is added, updated, or removed from a Service.
- **Connector**: Flows data into a Service (e.g., from a file, socket, etc.) via `Service.OnMessage()`, and can also publish data out from the Service.

Some connectors only **publish** (no incoming data), others only **subscribe** (outgoing data is ignored), and some do **both**.

By default, the system **generates 10,000 data points**. If you need more comprehensive testing, you can modify the `DATASIZE` value in `DataGenerator.hpp` from `10,000` to `1,000,000`.
