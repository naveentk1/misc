#include <iostream>
using namespace std;

// ============================================
// PATTERN PRINTING - ZOHO INTERVIEW GUIDE
// ============================================

// PATTERN 1: Square Pattern
// ****
// ****
// ****
// ****
void pattern1(int n) {
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      cout << "* ";
    }
    cout << endl;
  }
}

// PATTERN 2: Right-angled Triangle
// *
// **
// ***
// ****
void pattern2(int n) {
  for (int i = 1; i <= n; i++) {
    for (int j = 1; j <= i; j++) {
      cout << "* ";
    }
    cout << endl;
  }
}

// PATTERN 3: Inverted Right-angled Triangle
// ****
// ***
// **
// *
void pattern3(int n) {
  for (int i = n; i >= 1; i--) {
    for (int j = 1; j <= i; j++) {
      cout << "* ";
    }
    cout << endl;
  }
}

// PATTERN 4: Right-aligned Triangle (IMPORTANT!)
//    *
//   **
//  ***
// ****
void pattern4(int n) {
  for (int i = 1; i <= n; i++) {
    // Print spaces
    for (int j = 1; j <= n - i; j++) {
      cout << " ";
    }
    // Print stars
    for (int j = 1; j <= i; j++) {
      cout << "*";
    }
    cout << endl;
  }
}

// PATTERN 5: Pyramid (VERY COMMON IN ZOHO)
//    *
//   ***
//  *****
// *******
void pattern5(int n) {
  for (int i = 1; i <= n; i++) {
    // Print spaces
    for (int j = 1; j <= n - i; j++) {
      cout << " ";
    }
    // Print stars (odd numbers: 1, 3, 5, 7...)
    for (int j = 1; j <= 2 * i - 1; j++) {
      cout << "*";
    }
    cout << endl;
  }
}

// PATTERN 6: Inverted Pyramid
// *******
//  *****
//   ***
//    *
void pattern6(int n) {
  for (int i = n; i >= 1; i--) {
    // Print spaces
    for (int j = 1; j <= n - i; j++) {
      cout << " ";
    }
    // Print stars
    for (int j = 1; j <= 2 * i - 1; j++) {
      cout << "*";
    }
    cout << endl;
  }
}

// PATTERN 7: Diamond (SUPER IMPORTANT!)
//    *
//   ***
//  *****
// *******
//  *****
//   ***
//    *
void pattern7(int n) {
  // Upper half (pyramid)
  for (int i = 1; i <= n; i++) {
    for (int j = 1; j <= n - i; j++) {
      cout << " ";
    }
    for (int j = 1; j <= 2 * i - 1; j++) {
      cout << "*";
    }
    cout << endl;
  }
  // Lower half (inverted pyramid)
  for (int i = n - 1; i >= 1; i--) {
    for (int j = 1; j <= n - i; j++) {
      cout << " ";
    }
    for (int j = 1; j <= 2 * i - 1; j++) {
      cout << "*";
    }
    cout << endl;
  }
}

// PATTERN 8: Number Triangle
// 1
// 12
// 123
// 1234
void pattern8(int n) {
  for (int i = 1; i <= n; i++) {
    for (int j = 1; j <= i; j++) {
      cout << j << " ";
    }
    cout << endl;
  }
}

// PATTERN 9: Number Triangle (Row number)
// 1
// 22
// 333
// 4444
void pattern9(int n) {
  for (int i = 1; i <= n; i++) {
    for (int j = 1; j <= i; j++) {
      cout << i << " ";
    }
    cout << endl;
  }
}

// PATTERN 10: Floyd's Triangle (ASKED IN ZOHO!)
// 1
// 2 3
// 4 5 6
// 7 8 9 10
void pattern10(int n) {
  int num = 1;
  for (int i = 1; i <= n; i++) {
    for (int j = 1; j <= i; j++) {
      cout << num << " ";
      num++;
    }
    cout << endl;
  }
}

// PATTERN 11: Hollow Square
// ****
// *  *
// *  *
// ****
void pattern11(int n) {
  for (int i = 1; i <= n; i++) {
    for (int j = 1; j <= n; j++) {
      if (i == 1 || i == n || j == 1 || j == n) {
        cout << "* ";
      } else {
        cout << "  ";
      }
    }
    cout << endl;
  }
}

// PATTERN 12: Hollow Pyramid
//    *
//   * *
//  *   *
// *******
void pattern12(int n) {
  for (int i = 1; i <= n; i++) {
    // Spaces
    for (int j = 1; j <= n - i; j++) {
      cout << " ";
    }
    // Stars and spaces
    for (int j = 1; j <= 2 * i - 1; j++) {
      if (j == 1 || j == 2 * i - 1 || i == n) {
        cout << "*";
      } else {
        cout << " ";
      }
    }
    cout << endl;
  }
}

// PATTERN 13: Pascal's Triangle (ADVANCED)
//     1
//    1 1
//   1 2 1
//  1 3 3 1
// 1 4 6 4 1
void pattern13(int n) {
  for (int i = 0; i < n; i++) {
    int num = 1;
    // Spaces
    for (int j = 0; j < n - i - 1; j++) {
      cout << " ";
    }
    // Numbers
    for (int j = 0; j <= i; j++) {
      cout << num << " ";
      num = num * (i - j) / (j + 1);
    }
    cout << endl;
  }
}

int main() {
  int n = 5;

  cout << "Pattern 1: Square\n";
  pattern1(4);

  cout << "\nPattern 2: Right Triangle\n";
  pattern2(n);

  cout << "\nPattern 3: Inverted Triangle\n";
  pattern3(n);

  cout << "\nPattern 4: Right-aligned Triangle\n";
  pattern4(n);

  cout << "\nPattern 5: Pyramid\n";
  pattern5(n);

  cout << "\nPattern 6: Inverted Pyramid\n";
  pattern6(n);

  cout << "\nPattern 7: Diamond\n";
  pattern7(4);

  cout << "\nPattern 8: Number Triangle\n";
  pattern8(n);

  cout << "\nPattern 9: Row Number Triangle\n";
  pattern9(n);

  cout << "\nPattern 10: Floyd's Triangle\n";
  pattern10(n);

  cout << "\nPattern 11: Hollow Square\n";
  pattern11(n);

  cout << "\nPattern 12: Hollow Pyramid\n";
  pattern12(n);

  cout << "\nPattern 13: Pascal's Triangle\n";
  pattern13(n);

  return 0;
}

// ============================================
// THE GOLDEN FORMULA FOR PATTERNS:
// ============================================
// 1. Outer loop (i) = controls ROWS
// 2. Inner loops = controls COLUMNS
//    - First inner loop often for SPACES
//    - Second inner loop for STARS/NUMBERS
//
// For pyramids:
//    - Spaces = n - i
//    - Stars = 2*i - 1 (for symmetric pyramids)
// ============================================
