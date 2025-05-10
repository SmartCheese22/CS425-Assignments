# A2 - DNS Resolver

## Overview

This assignment implements a DNS resolution system in Python that supports both iterative and recursive lookups. The iterative approach manually queries DNS servers starting from the root servers and progresses through TLD and authoritative servers, while the recursive approach uses the system's default resolver.

## Changes Made

### Error Handling and Debugging Improvements

- **UDP Query Timeout Handling:**  
  In the `send_dns_query` function, a timeout is set using `dns.query.udp(..., timeout=TIMEOUT)`. This ensures that the query does not hang indefinitely if a server is unreachable.
- **Detailed Exception Handling:**  
  Both `send_dns_query` and `extract_next_nameservers` functions now include specific exception blocks for timeouts, NXDOMAIN (non-existent domains), and other generic exceptions. This provides clear, meaningful error messages to aid in debugging.
- **Hostname Cleanup:**  
  In `extract_next_nameservers`, nameserver hostnames are cleaned by stripping trailing dots to ensure that the DNS resolver works correctly.
- **Stage Progression:**  
  The `iterative_dns_lookup` function now includes a check to verify if any nameservers are found at each stage (ROOT, TLD, AUTH). If no nameservers are found, an error is logged and the lookup terminates.
- **Recursive Lookup Improvements:**  
  The `recursive_dns_lookup` function first retrieves NS records to verify delegation and then queries for A records. This two-step process provides more insight into the domain's DNS setup.

### Detailed Inline Comments

- Every change made after the TODO comments has been explained with detailed inline comments. This not only documents the purpose of the changes but also aids in understanding the flow and logic of the code.

## How to Run the Code

1. **Extract the Assignment Directory:**

2. **Navigate to the Assignment Directory:**

   ```bash
   cd A2_220285_220276_220638
   ```

3. **Run Iterative DNS Resolution:**

   ```bash
   python3 dns_server.py iterative example.com
   ```

4. **Run Recursive DNS Resolution:**

   ```bash
   python3 dns_server.py recursive example.com
   ```

   Replace `example.com` with any domain you wish to test.

## Testing

### Correctness Testing

- **Basic Functionality:**  
  Tested with known domains such as `google.com` and `example.com` to ensure that both iterative and recursive lookups return the correct A record responses.
- **Error Scenarios:**  
  Tested with non-existent domains (e.g., `nonexistentdomain12345.com`) to verify that the error messages (like NXDOMAIN and timeouts) are handled gracefully.

## Member Contributions

- **Prathamesh Baviskar (220285) (34%):**
  - Completed the iterative DNS lookup and other helper functions.
  - Implemented error handling.
- **Mayank Gupta (220638) (33%):**
  - Completed the recursive DNS resolution function.
- **Ayushmaan Jay Singh (220276) (33%):**
  - Wrote and formatted the README file.

## Sources Referred

- **dnspython Documentation:**  
  [dnspython Docs](https://www.dnspython.org/docs/)
- **Various Blogs and Forums:**  
  Relevant posts on Stack Overflow and technical blogs discussing DNS resolution, UDP communication, and error handling.

## Declaration

I/We hereby declare that this assignment is entirely our own work. No part of this assignment was copied from any external source without proper attribution, and we have not engaged in any form of plagiarism.

## Feedback

- **Overall Experience:**  
  The assignment was challenging and engaging, offering a great opportunity to explore the intricacies of DNS resolution and network programming.
