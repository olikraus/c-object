# dnfwebapp Makefile

# --- Build Configuration ---
CC_default = gcc
CC_amd64   = x86_64-linux-gnu-gcc
CC_arm64   = aarch64-linux-gnu-gcc

SUFFIX_default = 
SUFFIX_amd64   = _amd64
SUFFIX_arm64   = _arm64

INSTALL_amd64 = sudo apt update && sudo apt install gcc-x86-64-linux-gnu
INSTALL_arm64 = sudo apt update && sudo apt install gcc-aarch64-linux-gnu

# Detect local SIMD support for default host
SIMD_default =
ifeq ($(shell uname -m),x86_64)
SIMD_default = -msse4.2
endif
SIMD_amd64 = -msse4.2
SIMD_arm64 =

# --- Flags ---
CFLAGS_COMMON = -g -fpie -DXML_GE=1 -Wall -I../co -I../co/expat
LDFLAGS_COMMON = -pie -Wl,--as-needed -static-libgcc -Wl,-Bstatic -lm -lpthread -Wl,-Bdynamic

# --- Sources ---
COSRC = ../co/co.c ../co/co_dnf.c ../co/co_bitset.c
COMMON_SRC = dwa.c dwa_json.c dwa_op.c dwa_validate.c

# --- Targets ---
.PHONY: all cli cli_amd64 cli_arm64 cli_default cgi cgi_amd64 cgi_arm64 cgi_default aws aws_amd64 aws_arm64 aws_default \
        cli-test cli-perf-test cli_amd64-test cli_amd64-perf-test cli_arm64-test cli_arm64-perf-test \
        cgi-test cgi-perf-test cgi_amd64-test cgi_amd64-perf-test cgi_arm64-test cgi_arm64-perf-test \
        aws-test aws-perf-test aws_amd64-test aws_amd64-perf-test aws_arm64-test aws_arm64-perf-test \
        cli_default-test cli_default-perf-test cgi_default-test cgi_default-perf-test aws_default-test aws_default-perf-test \
        perf-test cgi-install aws-install clean

all: cli cli_amd64 cli_arm64 cgi cgi_amd64 cgi_arm64 aws aws_amd64 aws_arm64

# --- Rules Template ---
define HOST_RULES
OBJDIR_$(1) = obj$(SUFFIX_$(1))
OBJS_$(1) = $$(addprefix $$(OBJDIR_$(1))/, $$(notdir $$(COSRC:.c=.o) $$(COMMON_SRC:.c=.o)))

$$(OBJDIR_$(1))/%.o: %.c
	@mkdir -p $$(OBJDIR_$(1))
	$$(CC_$(1)) $$(CFLAGS_COMMON) $$(SIMD_$(1)) -c $$< -o $$@

$$(OBJDIR_$(1))/%.o: ../co/%.c
	@mkdir -p $$(OBJDIR_$(1))
	$$(CC_$(1)) $$(CFLAGS_COMMON) $$(SIMD_$(1)) -c $$< -o $$@

$$(OBJS_$(1)) $$(OBJDIR_$(1))/dwa_cli.o $$(OBJDIR_$(1))/dwa_cgi.o $$(OBJDIR_$(1))/dwa_aws_lambda.o: dnfwebapp.h

cli$(SUFFIX_$(1)): dnfwebapp$(SUFFIX_$(1)).cli
cgi$(SUFFIX_$(1)): dnfwebapp$(SUFFIX_$(1)).cgi
aws$(SUFFIX_$(1)): dnfwebapp$(SUFFIX_$(1)).aws

dnfwebapp$(SUFFIX_$(1)).cli: $$(OBJS_$(1)) $$(OBJDIR_$(1))/dwa_cli.o
	@if [ "$(1)" != "default" ] && [ -z "$$(shell which $$(CC_$(1)) 2>/dev/null)" ]; then \
		echo "Error: $(1) cross-compiler ($$(CC_$(1))) not found."; \
		echo "Install with: $$(INSTALL_$(1))"; \
		exit 1; \
	fi
	$$(CC_$(1)) $$(CFLAGS_COMMON) $$(SIMD_$(1)) $$^ -o $$@ $$(LDFLAGS_COMMON)

dnfwebapp$(SUFFIX_$(1)).cgi: $$(OBJS_$(1)) $$(OBJDIR_$(1))/dwa_cgi.o
	@if [ "$(1)" != "default" ] && [ -z "$$(shell which $$(CC_$(1)) 2>/dev/null)" ]; then \
		echo "Error: $(1) cross-compiler ($$(CC_$(1))) not found."; \
		echo "Install with: $$(INSTALL_$(1))"; \
		exit 1; \
	fi
	$$(CC_$(1)) $$(CFLAGS_COMMON) $$(SIMD_$(1)) $$^ -o $$@ $$(LDFLAGS_COMMON)

dnfwebapp$(SUFFIX_$(1)).aws: $$(OBJS_$(1)) $$(OBJDIR_$(1))/dwa_aws_lambda.o
	@if [ "$(1)" != "default" ] && [ -z "$$(shell which $$(CC_$(1)) 2>/dev/null)" ]; then \
		echo "Error: $(1) cross-compiler ($$(CC_$(1))) not found."; \
		echo "Install with: $$(INSTALL_$(1))"; \
		exit 1; \
	fi
	$$(CC_$(1)) $$(CFLAGS_COMMON) $$(SIMD_$(1)) $$^ -o $$@ $$(LDFLAGS_COMMON)
	chmod a+rwx $$@

cli$(SUFFIX_$(1))-test: cli$(SUFFIX_$(1))
	./dnfwebapp$(SUFFIX_$(1)).cli ../test/dwa_test_input.json -o tmp_test_res_$(1).json
	jq .result tmp_test_res_$(1).json > tmp_test_res_filtered_$(1).json
	jq .result ../test/dwa_expected_output.json > tmp_expected_filtered_$(1).json
	diff tmp_test_res_filtered_$(1).json tmp_expected_filtered_$(1).json
	rm tmp_test_res_$(1).json tmp_test_res_filtered_$(1).json tmp_expected_filtered_$(1).json

cli$(SUFFIX_$(1))-perf-test: cli$(SUFFIX_$(1)) perf_input.json
	@echo "--- CLI Performance Test ($(1)) ---"
	@/usr/bin/time -f "Total Time: %es" ./dnfwebapp$(SUFFIX_$(1)).cli perf_input.json -o perf_res_cli_$(1).json > /dev/null
	@echo "Internal Log:"
	@jq -r .log perf_res_cli_$(1).json
	@rm perf_res_cli_$(1).json

cgi$(SUFFIX_$(1))-test: cgi$(SUFFIX_$(1))
	@echo "Testing CGI ($(1)) with curl..."
	curl -X POST -H "Content-Type: application/json" --data-binary @../test/dwa_test_input.json http://localhost/cgi-bin/dnfwebapp$(SUFFIX_$(1)).cgi > tmp_cgi_res_$(1).json
	@echo "Comparing results..."
	jq .result tmp_cgi_res_$(1).json > tmp_cgi_res_filtered_$(1).json
	jq .result ../test/dwa_expected_output.json > tmp_expected_filtered_$(1).json
	diff tmp_cgi_res_filtered_$(1).json tmp_expected_filtered_$(1).json
	rm tmp_cgi_res_$(1).json tmp_cgi_res_filtered_$(1).json tmp_expected_filtered_$(1).json

cgi$(SUFFIX_$(1))-perf-test: cgi$(SUFFIX_$(1)) perf_input.json
	@echo "--- CGI Performance Test ($(1)) ---"
	@curl -s -X POST -H "Content-Type: application/json" \
		--data-binary @perf_input.json \
		-o perf_res_cgi_$(1).json \
		-w "Upload/Connect: %{time_pretransfer}s\nProcessing (TTFB): %{time_starttransfer}s\nTotal Time: %{time_total}s\n" \
		http://localhost/cgi-bin/dnfwebapp$(SUFFIX_$(1)).cgi
	@echo "Internal Log:"
	@jq -r .log perf_res_cgi_$(1).json
	@rm perf_res_cgi_$(1).json

aws$(SUFFIX_$(1))-test: aws$(SUFFIX_$(1)) perf_input.json venv
	@echo "Starting Flask Lambda Emulator ($(1))..."
	@fuser -k 8080/tcp || true
	@./venv/bin/python3 lambda_runtime_emulator.py > flask_emulator_$(1).log 2>&1 & \
	FLASK_PID=$$!; \
	sleep 2; \
	echo "Starting AWS Lambda Runtime binary ($(1))..."; \
	AWS_LAMBDA_RUNTIME_API=127.0.0.1:8080 ./dnfwebapp$(SUFFIX_$(1)).aws > aws_runtime_$(1).log 2>&1 & \
	RUNTIME_PID=$$!; \
	sleep 2; \
	echo "Triggering invocation..."; \
	curl -s -X POST "http://localhost:8080/2015-03-31/functions/function/invocations" -d @../test/dwa_test_input.json > tmp_aws_res_$(1).json; \
	kill $$RUNTIME_PID 2>/dev/null || true; \
	kill $$FLASK_PID 2>/dev/null || true; \
	echo "Comparing AWS results..."; \
	jq .result tmp_aws_res_$(1).json > tmp_aws_res_filtered_$(1).json; \
	jq .result ../test/dwa_expected_output.json > tmp_expected_filtered_$(1).json; \
	diff tmp_aws_res_filtered_$(1).json tmp_expected_filtered_$(1).json; \
	rm tmp_aws_res_$(1).json tmp_aws_res_filtered_$(1).json tmp_expected_filtered_$(1).json

aws$(SUFFIX_$(1))-perf-test: aws$(SUFFIX_$(1)) perf_input.json venv
	@echo "--- AWS Performance Test ($(1)) ---"
	@echo "Starting Flask Lambda Emulator..."
	@fuser -k 8080/tcp || true
	@./venv/bin/python3 lambda_runtime_emulator.py > flask_emulator_$(1).log 2>&1 & \
	FLASK_PID=$$!; \
	sleep 2; \
	echo "Starting AWS Lambda Runtime binary..."; \
	AWS_LAMBDA_RUNTIME_API=127.0.0.1:8080 ./dnfwebapp$(SUFFIX_$(1)).aws > aws_runtime_$(1).log 2>&1 & \
	RUNTIME_PID=$$!; \
	sleep 2; \
	echo "Triggering performance invocation..."; \
	curl -s -X POST "http://localhost:8080/2015-03-31/functions/function/invocations" -d @perf_input.json -o tmp_aws_perf_$(1).json \
		-w "Upload/Connect: %{time_pretransfer}s\nProcessing (TTFB): %{time_starttransfer}s\nTotal Time: %{time_total}s\n"; \
	kill $$RUNTIME_PID 2>/dev/null || true; \
	kill $$FLASK_PID 2>/dev/null || true; \
	echo "Internal Log:"; \
	jq -r .log tmp_aws_perf_$(1).json; \
	rm tmp_aws_perf_$(1).json
endef

$(eval $(call HOST_RULES,default))
$(eval $(call HOST_RULES,amd64))
$(eval $(call HOST_RULES,arm64))

# --- Aliases ---
cli_default: cli
cgi_default: cgi
aws_default: aws

cli_default-test: cli-test
cli_default-perf-test: cli-perf-test

cgi_default-test: cgi-test
cgi_default-perf-test: cgi-perf-test

aws_default-test: aws-test
aws_default-perf-test: aws-perf-test

perf-test: cli-perf-test cgi-perf-test aws-perf-test

perf_input.json:
	../dnf -gpsd 2 20 -gdnf 2 2 3 -gic 200000 1 -o perf_input.json

cgi-install: cgi
	cp dnfwebapp.cgi /usr/lib/cgi-bin/dnfwebapp.cgi
	chmod a+rwx /usr/lib/cgi-bin/dnfwebapp.cgi

aws-install: venv
	chmod a+rwx lambda_runtime_emulator.py

venv:
	python3 -m venv venv
	./venv/bin/pip install flask

clean:
	-rm -f dnfwebapp.cli dnfwebapp.cgi dnfwebapp.aws dnfwebapp_*.cli dnfwebapp_*.cgi dnfwebapp_*.aws
	-rm -rf obj obj_amd64 obj_arm64
	-rm -f aws_rie.log aws_runtime.log flask_emulator.log out.json perf_input.json tmp_aws_res.json
	-rm -rf venv tmp
