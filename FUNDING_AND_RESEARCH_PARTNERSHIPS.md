# BreadOS Funding and Research Partnership Plan

Updated: 2026-11-13

## Funding objective

Secure non-dilutive support for the research, engineering, security validation, hardware enablement, application compatibility, accessibility, documentation, release infrastructure, and long-term maintenance of BreadOS. BreadOS is intended to remain a not-for-profit public-interest operating-system project. Funding proposals should describe the work as open-source systems and cybersecurity research and infrastructure, not as a conventional venture-backed startup.

No award is secured by this plan. Eligibility and award decisions remain subject to each funder's rules and review.

## Highest-priority opportunity: NSF PESOSE

The National Science Foundation's Pathways to Enable Secure Open-Source Ecosystems (PESOSE), solicitation NSF 26-506, is the closest identified programme match.

- Track 2 supports establishing and expanding a sustainable open-source ecosystem, including governance, continuous development, security, privacy, contributor engagement, sustainability, and measurable evaluation.
- Track 3 supports improving the safety, security, and privacy of an existing open-source ecosystem, including vulnerability assessment, dependency review, secure development, monitoring, response, and recovery.
- The solicitation lists maximum awards of $1,500,000 for Track 2 and $1,500,000 for Track 3, for up to two years. These are ceilings, not guaranteed awards.
- The next listed full-proposal deadline is March 2, 2027, by 5 p.m. in the submitting organisation's local time.
- The September 1, 2026 deadline has passed.
- Track 2 and Track 3 project descriptions are limited to 15 pages.
- A proposal must point to the existing public open-source product and explain its actual development and testing status, dissemination, user and contributor base, the problem addressed, and why the team is qualified.
- Track 2 requires a plan for governance, licensing, continuous development, risk management, community growth, financial sustainability, and evaluation.
- Track 3 requires specified vulnerability classes, a development plan, annual milestones, and measurable security evaluation.
- The solicitation requires three to five letters of collaboration from third-party current users and/or contributors who are not directly related to the proposing team. Letters must explain past and future contributions. This is a material readiness requirement, not a formality.
- Track 1 is a planning award capped at $300,000 and is not a product-development award.

### Eligibility limitation

NSF PESOSE does not accept every nonprofit automatically. Eligible applicants include accredited US institutions of higher education; certain US non-academic nonprofits directly associated with educational or research activities; US-based for-profit research organisations; and state or local governments. The submitting organisation and principal investigator must satisfy the solicitation's detailed eligibility and PI restrictions.

Before preparing a proposal in JAPStudios' name, obtain written eligibility guidance from the NSF programme contact using the organisation's exact legal form, tax status, governing documents, and research activities. If JAPStudios does not qualify as the lead applicant, an eligible research institution could lead a genuinely collaborative proposal. The solicitation says ineligible organisations cannot receive NSF subawards; their participation would need non-NSF support. Do not assume that a university partnership automatically makes an otherwise ineligible organisation eligible.

Official solicitation: https://www.nsf.gov/funding/opportunities/pesose-pathways-enable-secure-open-source-ecosystems/nsf26-506/solicitation

## Additional research funding routes

### NSF Computer Systems Research

The NSF Computer Systems Research programme supports research into secure, reliable, efficient, sustainable computer systems, including operating and runtime systems, resource management, virtualization, and software infrastructure. It is a strong technical fit for defined research questions such as process isolation, memory safety, executable compatibility, storage integrity, and secure system architecture. The current solicitation and eligible-organisation rules must be checked before any application.

Official programme: https://www.nsf.gov/funding/opportunities/csr-computer-systems-research

### NSF Secure and Trustworthy Cyberspace

Use the current solicitation, not archived versions, to assess whether a defined BreadOS security research project fits. A proposal should ask a researchable question and provide a validation method, rather than simply request money to finish an entire operating system.

Current programme page: https://www.nsf.gov/funding/opportunities/secure-trustworthy-cyberspace-satc

### DHS Science and Technology Directorate

Monitor DHS S&T funding notices, Broad Agency Announcements, prize challenges, and research solicitations for specific cybersecurity, resilient computing, trusted software, or infrastructure needs. Submit only against a published topic whose scope and applicant eligibility match BreadOS. A general expression of interest is not a grant application and does not guarantee funding.

DHS S&T: https://www.dhs.gov/science-and-technology
Federal opportunities: https://simpler.grants.gov/

### Federal procurement

Separate grants from contracts. A contract requires an agency-defined need and a competitive procurement or other authorised acquisition route. Monitor SAM.gov opportunities for operating-system security, endpoint resilience, secure computing, software assurance, and research-and-development requirements. Do not represent a grant as a purchase contract or a contract as a grant.

SAM.gov: https://sam.gov/

## Routes that do not directly fit the current not-for-profit applicant

### SBIR and STTR

A nonprofit cannot directly receive an SBIR or STTR award. The federal SBIR programme requires the awardee to be an eligible for-profit small business concern. A nonprofit may participate as a research institution in an eligible STTR arrangement or as a subcontractor/subrecipient where programme rules permit, but it cannot be the direct SBIR/STTR awardee. Do not make a for-profit applicant or joint venture the default workaround; that would change the applicant and governance structure and must be independently acceptable.

Official eligibility FAQ: https://www.sbir.gov/faq/eligibility-requirements

## CISA Secure by Design

CISA's Secure by Design principles are useful as a security engineering and assurance framework: take responsibility for customer security outcomes, build security into design and defaults, and demonstrate transparency and accountability. The Secure by Design Pledge is voluntary and non-binding. Its stated scope is enterprise software products and services; consumer products are not within the pledge's defined scope. Treat the guidance as a benchmark for BreadOS, not as a source of funding or an automatic certification.

Pledge: https://www.cisa.gov/securebydesign/pledge

## Federal applicant registration

For federal grant applications, verify the organisation's exact eligibility first. Where the programme requires organisational registration, complete and maintain the entity's SAM.gov registration, obtain or verify its Unique Entity ID, designate the authorised entity representative, and complete the Grants.gov applicant profile. Registration is free and may take at least 7–10 business days after the required information is submitted; start well before a deadline.

Grants.gov registration guidance: https://www.grants.gov/applicants/applicant-registration/

## Proposal package to prepare

1. A technically accurate BreadOS description that separates implemented capabilities from planned capabilities.
2. A work breakdown structure covering kernel and process architecture, security boundaries, executable and application compatibility, graphics and desktop, hardware support, networking, storage, installer and updates, testing, and release engineering.
3. A milestone schedule with measurable acceptance tests for each work package.
4. A security plan covering threat modelling, vulnerability handling, dependency provenance, signed builds and updates, reproducible builds, software bills of materials, independent review, incident response, and recovery.
5. A budget tied to named work packages, personnel effort, hardware/test infrastructure, external security review, accessibility, documentation, and allowable indirect costs.
6. A licensing and third-party code provenance register.
7. A sustainability plan for maintenance after the award.
8. Independent letters of collaboration from real users, contributors, research partners, or test organisations, as required by the chosen programme.

## How to make the funding request credible

Do not promise universal compatibility, perfect malware prevention, or guaranteed neutralisation. State these as product objectives and explain the architecture and validation needed to approach them. Malware prevention requires enforceable process isolation, least privilege, secure boot and update chains, memory protections, application signing or trust policy, exploit mitigations, and independent adversarial testing. A permission setting or a rule that says software is unauthorised is not, by itself, an enforcement boundary.

Present current repository evidence: source code, build instructions, automated host tests, boot tests, documented known limitations, commit history, and a public issue/milestone tracker. Do not claim a successful build or boot until the corresponding run has been recorded.

## Taylor et al. v. Google LLC

Taylor et al. v. Google LLC, Case No. 5:20-cv-07956-VKD, is not a BreadOS grant or a source of project funding. It is a class-action settlement concerning alleged background cellular-data transfers by Android devices. Any settlement payment is governed by class membership and the settlement process; it is not an award to BreadOS and should not be included in the project's funding forecast.

Official settlement information: https://www.federalcellularclassaction.com/
