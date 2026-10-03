.PHONY: all clean help release debug android windows linux macos

# 定义所有预设
PRESETS = \
    android-arm64-debug android-arm64-release \
    android-arm32-debug android-arm32-release

# 按类型分组
RELEASE_PRESETS = $(filter %-release,$(PRESETS))
DEBUG_PRESETS = $(filter %-debug,$(PRESETS))

# 按平台分组
ANDROID_PRESETS = $(filter android-%,$(PRESETS))

# 组合目标
android-release: $(filter android-%-release,$(PRESETS))
android-debug: $(filter android-%-debug,$(PRESETS))

# x86_64 和 ARM64 分别编译
arm64: $(filter %-arm64-%,$(PRESETS))
arm32: $(filter %-arm32-%,$(PRESETS))

# 编译所有
all: $(PRESETS)

# 只编译 Release 版本
release: $(RELEASE_PRESETS)

# 只编译 Debug 版本
debug: $(DEBUG_PRESETS)

# 按平台编译
android: $(ANDROID_PRESETS)

# 通用编译规则
$(PRESETS):
	@echo "=========================================="
	@echo "Building: $@"
	@echo "=========================================="
	@cmake --preset $@ && cmake --build --preset $@
	@if [ $$? -eq 0 ]; then \
		echo "✓ Successfully built: $@"; \
	else \
		echo "✗ Failed to build: $@"; \
		exit 1; \
	fi
	@echo ""

# 并行编译所有（使用 -j 参数）
parallel:
	@for preset in $(PRESETS); do \
		echo "Building $$preset &"; \
		cmake --preset $$preset && cmake --build --preset $$preset & \
	done; \
	wait

# 清理
clean:
	@echo "Cleaning all build directories..."
	@rm -rf build
	@echo "✓ Clean complete"

# 查看帮助
help:
	@echo "Available targets:"
	@echo "  all                     - Build all configurations"
	@echo "  release                 - Build all Release configurations"
	@echo "  debug                   - Build all Debug configurations"
	@echo "  android                 - Build all Android configurations"
	@echo "  android-release         - Build all Android Release configurations"
	@echo "  android-debug           - Build all Android Debug configurations"
	@echo "  arm64                   - Build all ARM64 configurations"
	@echo "  arm32                   - Build all ARM32 configurations"
	@echo "  parallel                - Build all configurations in parallel"
	@echo "  clean                   - Remove all build directories"
	@echo ""
	@echo "Individual targets:"
	@printf "  %s\n" $(PRESETS)
