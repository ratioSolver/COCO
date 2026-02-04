import { App, Button, flick, Navbar, NavbarItem, NavbarList, OffcanvasBrand } from '@ratiosolver/flick';
import { h } from 'snabbdom';

// 1. Define your state
let count = 0;

// 2. Define actions that update state and trigger a redraw
function increment() {
  count++;
  flick.redraw();
}

// 3. Mount the application
flick.mount(() => {
  // The main render function returns the entire app view

  const navbar = Navbar(
    OffcanvasBrand('Counter App'),
    NavbarList([
      NavbarItem('Home', () => console.log('Navigating to Home'), true)
    ])
  );

  const content = h('div.container.mt-5.text-center', [
    h('h1', 'Welcome to Flick'),
    h('p.lead', `Current count is: ${count}`),
    Button('Increment', increment)
  ]);

  // The App component typically wraps the layout
  return App(navbar, content);
});
