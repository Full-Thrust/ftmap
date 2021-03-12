#!/bin/awk -f
##############################################################################
#
# example awk filer to process game reports to ftmap format data file
#
##############################################################################

#
# Load in header file removing the HEADER: tags this section doesn't change
# for different record formats
#
($1 == "HEADER:") {
  for (a=2 ; a<(NF+1) ; a++) {
    printf "%s ", $a;
  }
  printf "\n";
  next;
}

#
# Parse ship record fields - customise according to report format
#
/Ship:/ {
  ship_name = $2;
}

/Type:/ {
  ship_type = $2;
}

/Position:/ {
  ship_x = substr( $2, 1, index($2,";")-1 );
  ship_y = $3;
}

/Velocity:/ {
    ship_v = $2;
}

#
# For move mork out delta heading change from text string moves to
# Port are -ve moves to Starboard are +ve
#
# e.g. 2P = -2
#      2S = 2
#
/Move:/ {
  plot_string = $2;
  delta_h = substr(plot_string,1,1);
  if ((delta_h != 0) && (delta_h != "-") && (delta_h != "+")) {
      if (substr(plot_string,2,1)=="P")  {
	  delta_h = -delta_h;
      }
  }else{      
      delta_h = 0;  
  }
}

#
# Heading is record terminator print record to file
#
/Heading:/ {
    ship_h = $2;
    ship_f = $2;

    if ( ship_name != "" ) {
    print ship_name;
    ship_name = "";
    print ship_type;    
    print ship_x;
    print ship_y;
    print ship_h;
    print ship_f;
    print ship_v;
    print delta_h;
  }
}

#
# Section terminator
#
END {
  print "*";
}

##############################################################################



