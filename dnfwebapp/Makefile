# dnfwebapp Makefile

CC_AMD64 = gcc
CC_ARM64 = aarch64-linux-gnu-gcc

# Compiler check for ARM64
ARM64_CC_EXISTS := $(shell which $(CC_ARM64) 2>/dev/null)

# Common flags
CFLAGS_COMMON = -g -fpie -DXML_GE=1 -Wall -I../co -I../co/expat
LDFLAGS_COMMON = -pie -Wl,--as-needed -static-libgcc -Wl,-Bstatic -lm -lpthread -Wl,-Bdynamic


# SIMD flags
SIMD_CFLAGS_AMD64 = -msse4.2
SIMD_CFLAGS_ARM64 = 

COSRC = ../co/co.c ../co/co_dnf.c ../co/co_bitset.c
COMMON_SRC = dwa.c dwa_json.c dwa_op.c dwa_validate.c

.PHONY: all cli cgi aws cli-test cgi-test cgi-install aws-install cli-perf-test cgi-perf-test aws-perf-test perf-test clean help-arm64

all: amd64
ifneq ($(ARM64_CC_EXISTS),)
all: arm64
else
all: help-arm64
endif

amd64: cli_amd64 cgi_amd64 aws_amd64
arm64: cli_arm64 cgi_arm64 aws_arm64

# Binary Names
CLI_AMD64 = dnfwebapp_amd64
CGI_AMD64 = dnfwebapp_amd64.cgi
AWS_AMD64 = dnfwebapp_amd64.aws

CLI_ARM64 = dnfwebapp_arm64
CGI_ARM64 = dnfwebapp_arm64.cgi
AWS_ARM64 = dnfwebapp_arm64.aws

# --- AMD64 Rules ---

OBJ_AMD64 = $(addprefix obj_amd64/, $(notdir $(COSRC:.c=.o) $(COMMON_SRC:.c=.o)))

cli_amd64: $(CLI_AMD64)
cgi_amd64: $(CGI_AMD64)
aws_amd64: $(AWS_AMD64)

$(CLI_AMD64): $(OBJ_AMD64) obj_amd64/dwa_cli.o
	$(CC_AMD64) $(CFLAGS_COMMON) $(SIMD_CFLAGS_AMD64) $^ -o $@ $(LDFLAGS_COMMON)

$(CGI_AMD64): $(OBJ_AMD64) obj_amd64/dwa_cgi.o
	$(CC_AMD64) $(CFLAGS_COMMON) $(SIMD_CFLAGS_AMD64) $^ -o $@ $(LDFLAGS_COMMON)

$(AWS_AMD64): $(OBJ_AMD64) obj_amd64/dwa_aws_lambda.o
	$(CC_AMD64) $(CFLAGS_COMMON) $(SIMD_CFLAGS_AMD64) $^ -o $@ $(LDFLAGS_COMMON)
	chmod a+rwx $@

obj_amd64/%.o: %.c dnfwebapp.h
	@mkdir -p obj_amd64
	$(CC_AMD64) $(CFLAGS_COMMON) $(SIMD_CFLAGS_AMD64) -c $< -o $@

obj_amd64/%.o: ../co/%.c dnfwebapp.h
	@mkdir -p obj_amd64
	$(CC_AMD64) $(CFLAGS_COMMON) $(SIMD_CFLAGS_AMD64) -c $< -o $@

# --- ARM64 Rules ---

OBJ_ARM64 = $(addprefix obj_arm64/, $(notdir $(COSRC:.c=.o) $(COMMON_SRC:.c=.o)))

cli_arm64: $(CLI_ARM64)
cgi_arm64: $(CGI_ARM64)
aws_arm64: $(AWS_ARM64)

$(CLI_ARM64): $(OBJ_ARM64) obj_arm64/dwa_cli.o
	$(CC_ARM64) $(CFLAGS_COMMON) $(SIMD_CFLAGS_ARM64) $^ -o $@ $(LDFLAGS_COMMON)

$(CGI_ARM64): $(OBJ_ARM64) obj_arm64/dwa_cgi.o
	$(CC_ARM64) $(CFLAGS_COMMON) $(SIMD_CFLAGS_ARM64) $^ -o $@ $(LDFLAGS_COMMON)

$(AWS_ARM64): $(OBJ_ARM64) obj_arm64/dwa_aws_lambda.o
	$(CC_ARM64) $(CFLAGS_COMMON) $(SIMD_CFLAGS_ARM64) $^ -o $@ $(LDFLAGS_COMMON)
	chmod a+rwx $@

obj_arm64/%.o: %.c dnfwebapp.h
	@mkdir -p obj_arm64
	$(CC_ARM64) $(CFLAGS_COMMON) $(SIMD_CFLAGS_ARM64) -c $< -o $@

obj_arm64/%.o: ../co/%.c dnfwebapp.h
	@mkdir -p obj_arm64
	$(CC_ARM64) $(CFLAGS_COMMON) $(SIMD_CFLAGS_ARM64) -c $< -o $@

# --- Helpers ---

help-arm64:
	@echo "Warning: ARM64 cross-compiler ($(CC_ARM64)) not found."
	@echo "To install on Ubuntu/Debian: sudo apt update && sudo apt install gcc-aarch64-linux-gnu"

# --- Testing (defaulting to AMD64) ---

cli: cli_amd64
cgi: cgi_amd64
aws: aws_amd64

cli-test: cli_amd64
	./$(CLI_AMD64) ../test/dwa_test_input.json -o tmp_test_res.json
	jq .result tmp_test_res.json > tmp_test_res_filtered.json
	jq .result ../test/dwa_expected_output.json > tmp_expected_filtered.json
	diff tmp_test_res_filtered.json tmp_expected_filtered.json
	rm tmp_test_res.json tmp_test_res_filtered.json tmp_expected_filtered.json

cgi-test: cgi_amd64
	@echo "Testing CGI with curl..."
	curl -X POST -H "Content-Type: application/json" --data-binary @../test/dwa_test_input.json http://localhost/cgi-bin/$(CGI_AMD64) > tmp_cgi_res.json
	@echo "Comparing results..."
	jq .result tmp_cgi_res.json > tmp_cgi_res_filtered.json
	jq .result ../test/dwa_expected_output.json > tmp_expected_filtered.json
	diff tmp_cgi_res_filtered.json tmp_expected_filtered.json
	rm tmp_cgi_res.json tmp_cgi_res_filtered.json tmp_expected_filtered.json

cgi-install: cgi_amd64
	cp $(CGI_AMD64) /usr/lib/cgi-bin/$(CGI_AMD64)
	chmod a+rwx /usr/lib/cgi-bin/$(CGI_AMD64)

aws-install: venv
	chmod a+rwx lambda_runtime_emulator.py

venv:
	python3 -m venv venv
	./venv/bin/pip install flask

aws-test: aws_amd64 perf_input.json venv
	@echo "Starting Flask Lambda Emulator..."
	@fuser -k 8080/tcp || true
	@./venv/bin/python3 lambda_runtime_emulator.py > flask_emulator.log 2>&1 & \
	FLASK_PID=$$!; \
	sleep 2; \
	echo "Starting AWS Lambda Runtime binary..."; \
	AWS_LAMBDA_RUNTIME_API=127.0.0.1:8080 ./$(AWS_AMD64) > aws_runtime.log 2>&1 & \
	RUNTIME_PID=$$!; \
	sleep 2; \
	echo "Triggering invocation..."; \
	curl -s -X POST "http://localhost:8080/2015-03-31/functions/function/invocations" -d @../test/dwa_test_input.json > tmp_aws_res.json; \
	kill $$RUNTIME_PID 2>/dev/null || true; \
	kill $$FLASK_PID 2>/dev/null || true; \
	echo "Comparing AWS results..."; \
	jq .result tmp_aws_res.json > tmp_aws_res_filtered.json; \
	jq .result ../test/dwa_expected_output.json > tmp_expected_filtered.json; \
	diff tmp_aws_res_filtered.json tmp_expected_filtered.json; \
	rm tmp_aws_res.json tmp_aws_res_filtered.json tmp_expected_filtered.json

aws-perf-test: aws_amd64 perf_input.json venv
	@echo "--- AWS Performance Test ---"
	@echo "Starting Flask Lambda Emulator..."
	@fuser -k 8080/tcp || true
	@./venv/bin/python3 lambda_runtime_emulator.py > flask_emulator.log 2>&1 & \
	FLASK_PID=$$!; \
	sleep 2; \
	echo "Starting AWS Lambda Runtime binary..."; \
	AWS_LAMBDA_RUNTIME_API=127.0.0.1:8080 ./$(AWS_AMD64) > aws_runtime.log 2>&1 & \
	RUNTIME_PID=$$!; \
	sleep 2; \
	echo "Triggering performance invocation..."; \
	curl -s -X POST "http://localhost:8080/2015-03-31/functions/function/invocations" -d @perf_input.json -o tmp_aws_perf.json \
		-w "Upload/Connect: %{time_pretransfer}s\nProcessing (TTFB): %{time_starttransfer}s\nTotal Time: %{time_total}s\n"; \
	kill $$RUNTIME_PID 2>/dev/null || true; \
	kill $$FLASK_PID 2>/dev/null || true; \
	echo "Internal Log:"; \
	jq -r .log tmp_aws_perf.json; \
	rm tmp_aws_perf.json

perf_input.json:
	../dnf -gpsd 2 20 -gdnf 2 2 3 -gic 200000 1 -o perf_input.json

cli-perf-test: cli_amd64 perf_input.json
	@echo "--- CLI Performance Test ---"
	@/usr/bin/time -f "Total Time: %es" ./$(CLI_AMD64) perf_input.json -o perf_res_cli.json > /dev/null
	@echo "Internal Log:"
	@jq -r .log perf_res_cli.json
	@rm perf_res_cli.json

cgi-perf-test: cgi_amd64 perf_input.json
	@echo "--- CGI Performance Test ---"
	@curl -s -X POST -H "Content-Type: application/json" \
		--data-binary @perf_input.json \
		-o perf_res_cgi.json \
		-w "Upload/Connect: %{time_pretransfer}s\nProcessing (TTFB): %{time_starttransfer}s\nTotal Time: %{time_total}s\n" \
		http://localhost/cgi-bin/$(CGI_AMD64)
	@echo "Internal Log:"
	@jq -r .log perf_res_cgi.json
	@rm perf_res_cgi.json

perf-test: cli-perf-test cgi-perf-test aws-perf-test

clean:
	-rm -f $(CLI_AMD64) $(CGI_AMD64) $(AWS_AMD64) $(CLI_ARM64) $(CGI_ARM64) $(AWS_ARM64)
	-rm -rf obj_amd64 obj_arm64
	-rm -f aws_rie.log aws_runtime.log flask_emulator.log out.json perf_input.json tmp_aws_res.json
	-rm -rf venv tmp
