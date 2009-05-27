



void backtick()
	receive char;
	send 	char;
{
  char c;
  
  while (1)
  {
    accept c;
    if (c != '`')
    {
      send c;
      continue;
    }
    
    accept c;
    if (c != '`')
    {
      send '`';
      send c;
      continue;
    }
    
    foreach c in "&ldquo;"
      send c;
  }
}

void tick()
	receive char;
	send char;
{
  char c;
  
  while(1)
  {
    accept c;
    if (c != '\'')
    {
      send c;
      continue;
    }
    
    accept c;
    if (c != '\'')
    {
      send '\'';
      send c;
      continue;
    }
    
    foreach c in "&rdquo;"
      send c;
  }
}


	input => backtick() => tick() => output;
	
	
	
	
	input => (*state)() => output;
	

	void (*state)() receive char; send char; [];
	
	state[0] = backtick;
	state[1] = tick;
	state[2] = mdash;
	state[3] = ndash;
	state[4] = hellip;
	state[5] = rarr;
	
	input => (*state)() => output;

void (*state)() receive char; send char;

void backtick1() receive char; send char;
{
  accept c;
  if (c != '`')
    send c;
  else 
    state = backtick2;
}

void backtick2() receive char; send char;
{
  accept c;
  if (c != '`')
  {
    send '`';
    send c;
  }
  else
  {
    foreach c in "&ldquo;"
      send c;
  }
  state = backtick1; 
}


