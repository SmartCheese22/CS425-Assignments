import dns.message
import dns.query
import dns.rdatatype
import dns.resolver
import time

# Root DNS servers used to start the iterative resolution process
ROOT_SERVERS = {
    "198.41.0.4": "Root (a.root-servers.net)",
    "199.9.14.201": "Root (b.root-servers.net)",
    "192.33.4.12": "Root (c.root-servers.net)",
    "199.7.91.13": "Root (d.root-servers.net)",
    "192.203.230.10": "Root (e.root-servers.net)"
}

TIMEOUT = 3  # Timeout in seconds for each DNS query attempt

def send_dns_query(server, domain):
    """ 
    Sends a DNS query to the given server for an A record of the specified domain.
    Returns the response if successful, otherwise returns None.
    """
    try:
        query = dns.message.make_query(domain, dns.rdatatype.A)  # Construct the DNS query
        # TODO: Send the query using UDP 
        # Completed: We use dns.query.udp to send the query to the server with a specified timeout.
        # This ensures the program does not wait indefinitely if the server does not respond.
        response = dns.query.udp(query, server, timeout=TIMEOUT)
        return response
    except dns.exception.Timeout:
        # Detailed error handling for timeouts:
        # If the UDP query does not receive a response within the TIMEOUT period, a Timeout exception is raised.
        # We catch this exception and print a meaningful error message.
        print(f"[ERROR] Timeout occurred when querying {server} for {domain}")
        return None
    except Exception as e:
        # Catch all other exceptions that may occur (e.g., network errors).
        # This helps in diagnosing issues by printing the exception message.
        print(f"[ERROR] Exception when querying {server} for {domain}: {e}")
        return None  # Return None to indicate the query failed.

def extract_next_nameservers(response):
    """ 
    Extracts nameserver (NS) records from the authority section of the response.
    Then, resolves those NS names to IP addresses.
    Returns a list of IPs of the next authoritative nameservers.
    """
    ns_ips = []  # List to store resolved nameserver IPs
    ns_names = []  # List to store nameserver domain names
    
    # Loop through the authority section to extract NS records
    for rrset in response.authority:
        if rrset.rdtype == dns.rdatatype.NS:
            for rr in rrset:
                ns_name = rr.to_text().strip('.')  # Removing trailing dot if any
                ns_names.append(ns_name)  # Extract nameserver hostname
                print(f"Extracted NS hostname: {ns_name}")

    # TODO: Resolve the extracted NS hostnames to IP addresses
    # Completed: We now loop over each extracted nameserver hostname and attempt to resolve it to its IP.
    # Detailed error handling is added to catch different types of resolution failures.
    for ns_name in ns_names:
        try:
            # Use dns.resolver.resolve to query the A record for the nameserver.
            # The lifetime parameter ensures that the query does not hang.
            answer = dns.resolver.resolve(ns_name, "A", lifetime=TIMEOUT)
            for rdata in answer:
                ns_ip = rdata.address
                ns_ips.append(ns_ip)
                # Log the successful resolution of the nameserver hostname to its IP.
                print(f"Resolved {ns_name} to {ns_ip}")
        except dns.resolver.NXDOMAIN:
            # If the domain does not exist, log an error message specifically for NXDOMAIN.
            print(f"[ERROR] NXDOMAIN: {ns_name} does not exist")
        except dns.resolver.Timeout:
            # If the resolution times out, log an appropriate error message.
            print(f"[ERROR] Timeout while resolving NS hostname: {ns_name}")
        except Exception as e:
            # For any other error, print a generic error message including the exception detail.
            print(f"[ERROR] Failed to resolve {ns_name}: {e}")
    
    # Return the list of successfully resolved nameserver IPs.
    return ns_ips

def iterative_dns_lookup(domain):
    """ 
    Performs an iterative DNS resolution starting from root servers.
    It queries root servers, then TLD servers, then authoritative servers,
    following the hierarchy until an answer is found or resolution fails.
    """
    print(f"[Iterative DNS Lookup] Resolving {domain}")

    next_ns_list = list(ROOT_SERVERS.keys())  # Start with the root server IPs
    stage = "ROOT"  # Track resolution stage (ROOT, TLD, AUTH)

    while next_ns_list:
        ns_ip = next_ns_list[0]  # Pick the first available nameserver to query
        response = send_dns_query(ns_ip, domain)
        
        if response: #checks if response is not NONE
            print(f"[DEBUG] Querying {stage} server ({ns_ip}) - SUCCESS")
            
            # If an answer is found, print and return
            if response.answer:
                print(f"[SUCCESS] {domain} -> {response.answer[0][0]}")
                return
            
            # If no answer, extract the next set of nameservers
            next_ns_list = extract_next_nameservers(response)
            if not next_ns_list:
                print(f"[ERROR] No NS records found during {stage} stage for {domain}")
                return
            # TODO: Move to the next resolution stage, i.e., it is either TLD, ROOT, or AUTH
            # Completed: Advance the resolution stage.
            # If the current stage is ROOT, change to TLD; if TLD, change to AUTH.
            if stage == "ROOT":
                stage = "TLD"
            elif stage == "TLD":
                stage = "AUTH"
        else:
            # If the query did not return a response, log the failure and stop the resolution process.
            print(f"[ERROR] Query failed for {stage} server {ns_ip}")
            return
    
    # If the loop ends without resolving the domain, print a final error message.
    print("[ERROR] Resolution failed.")

def recursive_dns_lookup(domain):
    """ 
    Performs recursive DNS resolution using the system's default resolver.
    This approach relies on a resolver (like Google DNS or a local ISP resolver)
    to fetch the result recursively.
    """
    print(f"[Recursive DNS Lookup] Resolving {domain}")
    try:
        # TODO: Perform recursive resolution using the system's DNS resolver
        # Notice that the next line is looping through, therefore you should have something like answer = ??
        # Completed: We perform recursive resolution in two steps:
        # 1. Resolve NS records to verify delegation and confirm the domain's setup.
        ns_answer = dns.resolver.resolve(domain, "NS", lifetime=TIMEOUT)
        for rdata in ns_answer:
            # Log each NS record received from the recursive lookup.
            print(f"[SUCCESS] NS record for {domain} -> {rdata}")
        
        # 2. Resolve the A record for the domain to get the final IP address.
        a_answer = dns.resolver.resolve(domain, "A", lifetime=TIMEOUT)
        for rdata in a_answer:
            # Log the successful resolution of the domain to its IP address.
            print(f"[SUCCESS] {domain} -> {rdata}")
    except dns.resolver.NXDOMAIN:
        # Specific handling for non-existent domains.
        print(f"[ERROR] Domain {domain} does not exist (NXDOMAIN)")
    except dns.resolver.Timeout:
        # Specific handling for timeout errors.
        print(f"[ERROR] Timeout occurred while resolving {domain}")
    except Exception as e:
        # Catch any other errors during the recursive resolution.
        print(f"[ERROR] Recursive lookup failed: {e}")  # Log the exception details.

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 3 or sys.argv[1] not in {"iterative", "recursive"}:
        print("Usage: python3 dns_server.py <iterative|recursive> <domain>")
        sys.exit(1)

    mode = sys.argv[1]  # Get mode (iterative or recursive)
    domain = sys.argv[2]  # Get domain to resolve
    start_time = time.time()  # Record start time
    
    # Execute the selected DNS resolution mode
    if mode == "iterative":
        iterative_dns_lookup(domain)
    else:
        recursive_dns_lookup(domain)
    
    print(f"Time taken: {time.time() - start_time:.3f} seconds")  # Print execution time