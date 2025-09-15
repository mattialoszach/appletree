# === Compile & Flags ===
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude

# === Project Files ===
SRCS = main.cpp
TARGET = appletree

# === Installation directory (User-local!) ===
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

# === Default: compile + build ===
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

# === Install executable in ~/.local/bin ===
install: appletree
	sudo cp appletree $(BINDIR)/
	sudo chmod +x $(BINDIR)/appletree
	@echo "✅ Installed to $(BINDIR)/appletree"

# === Remove executable from ~/.local/bin ===
uninstall:
	@rm -f $(BINDIR)/$(TARGET)
	@echo "🗑️  Uninstalled from $(BINDIR)"

# === Remove binaries from project directory ===
clean:
	@rm -f $(TARGET)
	@echo "🧹 Cleaned build artifacts"

# === Optional: run immediately (z. B. für dev) ===
run: $(TARGET)
	./$(TARGET)