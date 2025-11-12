# plugins/sma_python/sma_python.py
class Plugin:
    def __init__(self, ctx_capsule):
        # ctx_capsule is a py::capsule containing PluginContext*, not used here
        self.fast = 10
        self.slow = 30
        self.symbol = ""
        self.closes = []
        self.long_on = False
        self.entry = 0.0
        self.pnl = 0.0

    def init(self, options: dict) -> int:
        self.fast = int(options.get("fast", self.fast))
        self.slow = int(options.get("slow", self.slow))
        self.symbol = options.get("symbol", self.symbol)
        return 0

    def prepare_data(self, symbol: str, start_ns: int, end_ns: int, bar_sec: int) -> int:
        return 0

    def on_bar(self, ts_ns: int, o: float, h: float, l: float, c: float, v: float) -> int:
        self.closes.append(c)
        if len(self.closes) < max(self.fast, self.slow):
            return 0
        f = sum(self.closes[-self.fast:]) / self.fast
        s = sum(self.closes[-self.slow:]) / self.slow
        if not self.long_on and f > s:
            self.long_on = True
            self.entry = c
        elif self.long_on and f < s:
            self.pnl += (c - self.entry)
            self.long_on = False
        return 0

    def on_tick(self, ts_ns: int, price: float, size: float) -> int:
        return 0

    def finalize(self) -> str:
        if self.long_on and self.closes:
            self.pnl += (self.closes[-1] - self.entry)
            self.long_on = False
        return f'{{"symbol":"{self.symbol}","pnl":{self.pnl},"fast":{self.fast},"slow":{self.slow}}}'

def create_plugin(ctx_capsule):
    return Plugin(ctx_capsule)
