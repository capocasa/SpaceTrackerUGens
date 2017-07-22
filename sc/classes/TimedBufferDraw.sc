
TimedBufferDraw {
  var
    <>data,
    <>clump,
    <>title,
    <>window,
    <>res,
    <>length
  ;
  *new {
    // data: string in SpaceTracker format to visualize
    arg data, clump = 3, title = "";
    ^super.newCopyArgs(data, clump, title).init;
  }

  init {
    window = Window.new.front;
    window.view.background_(Color.white);

    length = data.collect{|a| a.clump(clump).collect{|e|e[0]}.sum}.maxItem;
    
    window.drawFunc = {

      var outerBounds, innerBounds, rect, offset;
      
      outerBounds = Rect(30, 30, window.view.bounds.width-60+30, window.view.bounds.height-60);

      innerBounds = outerBounds + Rect(45, 45, -45, -45);
      

      res = Point(
        innerBounds.width / data.size,
        innerBounds.height / length
      );
      
      rect = Rect();
      rect.width = res.x - 30;
      
      Pen.fillColor = Color.grey(0.6);
      Pen.strokeColor = Color.grey(0.7);

      Pen.stringCenteredIn(title, Rect(outerBounds.left, outerBounds.top, outerBounds.width, 20), Font( "Helvetica-Bold", 20), Color.grey(0.1));

      Pen.width=0.5;
      data.size.do { |i|
        Pen.stringCenteredIn(i.asString, Rect(innerBounds.left+(res.x*i), innerBounds.top-20, res.x, 15));
      };
      Pen.stroke;

      block {
        var step, offset;
        step = 0.25;
        offset = 0;
        
        while {
          var from, to;
          to = Point(innerBounds.left-20,innerBounds.top+(offset*res.y));
          from = Point(innerBounds.left+innerBounds.width+10, to.y);
          Pen.line(from, to);
          Pen.stroke;
          Pen.stringRightJustIn(offset.round(0.01).asString, Rect(to.x-35-10, to.y-7, 30, 14));
          offset = offset + step;
          offset < length;
        };

      };
 
      Pen.strokeColor = Color.grey(0.6);
      
      Pen.width=2;
      data.do { |column, i|
        offset = 0;
        rect.left = innerBounds.left + (i*res.x);
        column.clump(clump).do {
          |line|
          rect.top = innerBounds.top + (offset*res.y);
          rect.height = line[0]*res.y-1;
          
          if(line[1]==0) {
            Pen.fillColor = Color.grey(0.85);
            Pen.fillRect(rect);
          }{
            Pen.fillColor = Color.grey(0.95);
            Pen.fillRect(rect);
            Pen.strokeRect(rect);
          };
          offset = offset + line[0];
        };
      };   

    };
    window.refresh;
  }

  close {
    window.close;
  }
}


