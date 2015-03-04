if(!ENABLED) 
  h->take_exception(HWACHA_CAUSE_ILLEGAL_INSTRUCTION, VF_PC);

require_supervisor_hwacha;
xd = h->get_aux();
