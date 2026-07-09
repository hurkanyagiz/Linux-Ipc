# ═══════════════════════════════════════════════
#  Linux IPC Guide — Makefile
# ═══════════════════════════════════════════════

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c11
BUILD   = build

.PHONY: all clean pipe mqueue shm semaphore socket signal project

all: pipe mqueue shm semaphore socket signal project
	@echo ""
	@echo "All examples built successfully in ./$(BUILD)/"

$(BUILD):
	@mkdir -p $(BUILD)

# ── 1. Pipe ──
pipe: $(BUILD)
	@$(CC) $(CFLAGS) -o $(BUILD)/01_basic_pipe        src/01_pipe/01_basic_pipe.c
	@$(CC) $(CFLAGS) -o $(BUILD)/02_bidirectional_pipe src/01_pipe/02_bidirectional_pipe.c
	@$(CC) $(CFLAGS) -o $(BUILD)/03_pipe_redirect      src/01_pipe/03_pipe_redirect.c
	@$(CC) $(CFLAGS) -o $(BUILD)/04_named_pipe_writer   src/01_pipe/04_named_pipe_writer.c
	@$(CC) $(CFLAGS) -o $(BUILD)/05_named_pipe_reader   src/01_pipe/05_named_pipe_reader.c
	@echo "[OK] Pipe examples"

# ── 2. Message Queue ──
mqueue: $(BUILD)
	@$(CC) $(CFLAGS) -o $(BUILD)/01_mq_basic_send    src/02_message_queue/01_mq_basic_send.c    -lrt
	@$(CC) $(CFLAGS) -o $(BUILD)/02_mq_basic_receive  src/02_message_queue/02_mq_basic_receive.c -lrt
	@$(CC) $(CFLAGS) -o $(BUILD)/03_mq_priority        src/02_message_queue/03_mq_priority.c      -lrt
	@$(CC) $(CFLAGS) -o $(BUILD)/04_mq_notify          src/02_message_queue/04_mq_notify.c        -lrt
	@echo "[OK] Message Queue examples"

# ── 3. Shared Memory ──
shm: $(BUILD)
	@$(CC) $(CFLAGS) -o $(BUILD)/01_shm_write          src/03_shared_memory/01_shm_write.c          -lrt
	@$(CC) $(CFLAGS) -o $(BUILD)/02_shm_read            src/03_shared_memory/02_shm_read.c           -lrt
	@$(CC) $(CFLAGS) -o $(BUILD)/03_shm_struct          src/03_shared_memory/03_shm_struct.c         -lrt
	@$(CC) $(CFLAGS) -o $(BUILD)/04_shm_with_semaphore  src/03_shared_memory/04_shm_with_semaphore.c -lrt -lpthread
	@echo "[OK] Shared Memory examples"

# ── 4. Semaphore ──
semaphore: $(BUILD)
	@$(CC) $(CFLAGS) -o $(BUILD)/01_sem_basic              src/04_semaphore/01_sem_basic.c             -lpthread -lrt
	@$(CC) $(CFLAGS) -o $(BUILD)/02_sem_producer_consumer   src/04_semaphore/02_sem_producer_consumer.c -lpthread
	@$(CC) $(CFLAGS) -o $(BUILD)/03_sem_resource_pool       src/04_semaphore/03_sem_resource_pool.c     -lpthread
	@echo "[OK] Semaphore examples"

# ── 5. Socket ──
socket: $(BUILD)
	@$(CC) $(CFLAGS) -o $(BUILD)/01_tcp_server          src/05_socket/01_tcp_server.c
	@$(CC) $(CFLAGS) -o $(BUILD)/02_tcp_client          src/05_socket/02_tcp_client.c
	@$(CC) $(CFLAGS) -o $(BUILD)/03_unix_domain_server  src/05_socket/03_unix_domain_server.c
	@$(CC) $(CFLAGS) -o $(BUILD)/04_unix_domain_client  src/05_socket/04_unix_domain_client.c
	@$(CC) $(CFLAGS) -o $(BUILD)/05_udp_echo            src/05_socket/05_udp_echo.c
	@echo "[OK] Socket examples"

# ── 6. Signal ──
signal: $(BUILD)
	@$(CC) $(CFLAGS) -o $(BUILD)/01_basic_handler            src/06_signal/01_basic_handler.c
	@$(CC) $(CFLAGS) -o $(BUILD)/02_signal_between_processes  src/06_signal/02_signal_between_processes.c
	@$(CC) $(CFLAGS) -o $(BUILD)/03_timer_alarm               src/06_signal/03_timer_alarm.c
	@$(CC) $(CFLAGS) -o $(BUILD)/04_graceful_shutdown          src/06_signal/04_graceful_shutdown.c -lrt
	@echo "[OK] Signal examples"

# ── 7. Capstone Project ──
project: $(BUILD)
	@$(CC) $(CFLAGS) -o $(BUILD)/task_dispatcher \
		src/07_capstone_project/src/task_dispatcher.c -lrt -lpthread
	@$(CC) $(CFLAGS) -o $(BUILD)/task_worker \
		src/07_capstone_project/src/task_worker.c     -lrt -lpthread -lm
	@$(CC) $(CFLAGS) -o $(BUILD)/task_client \
		src/07_capstone_project/src/task_client.c
	@echo "[OK] Capstone project"
	@echo ""
	@echo "  To run the capstone project:"
	@echo "    Terminal 1: ./$(BUILD)/task_dispatcher"
	@echo "    Terminal 2: ./$(BUILD)/task_worker 1"
	@echo "    Terminal 3: ./$(BUILD)/task_worker 2"
	@echo "    Terminal 4: ./$(BUILD)/task_client"

clean:
	@rm -rf $(BUILD)
	@echo "[OK] Cleaned"
