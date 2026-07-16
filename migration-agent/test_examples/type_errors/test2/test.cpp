#define ATOMIC_READ(P) __sync_add_and_fetch((P), 0)

typedef enum RagentExectutorState {
    RAGENT_EXEC_INIT = 0x01,
    RAGENT_EXEC_PREPARE_METADATA,
} RagentExectutorState;

typedef struct RAgentGlobals {
RagentExectutorState executor_state;
} RAgentGlobals;

extern RAgentGlobals* g_agent_globals;

void test() {
  RagentExectutorState curr_state;
  curr_state = ATOMIC_READ(&g_agent_globals->executor_state);
}

