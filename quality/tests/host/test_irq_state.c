#include "spinlock.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

static bool interrupts_enabled;

void arch_cpu_relax(void) {}

hal_irq_state_t hal_irq_save_disable(void) {
  hal_irq_state_t state = interrupts_enabled ? 1U : 0U;
  interrupts_enabled = false;
  return state;
}

void hal_irq_restore(hal_irq_state_t state) {
  interrupts_enabled = state != 0U;
}

static void test_nested_save_restore(void) {
  interrupts_enabled = true;
  hal_irq_state_t outer = hal_irq_save_disable();
  hal_irq_state_t inner = hal_irq_save_disable();

  assert(!interrupts_enabled);
  hal_irq_restore(inner);
  assert(!interrupts_enabled);
  hal_irq_restore(outer);
  assert(interrupts_enabled);
}

static void test_disabled_entry_remains_disabled(void) {
  interrupts_enabled = false;
  hal_irq_state_t state = hal_irq_save_disable();
  hal_irq_restore(state);
  assert(!interrupts_enabled);
}

static void test_spin_lock_irqsave_nesting(void) {
  spinlock_t lock;
  spin_lock_init(&lock);
  interrupts_enabled = false;

  hal_irq_state_t state;
  spin_lock_irqsave(&lock, &state);
  assert(!interrupts_enabled);
  spin_unlock_irqrestore(&lock, state);
  assert(!interrupts_enabled);
}

int main(void) {
  test_nested_save_restore();
  test_disabled_entry_remains_disabled();
  test_spin_lock_irqsave_nesting();
  puts("IRQ state nesting tests passed");
  return 0;
}
